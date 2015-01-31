// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/viewport_surface.h"

#include "base/bind.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/services/surfaces/public/cpp/surfaces_utils.h"
#include "ui/gfx/transform.h"

using mojo::Size;
using mojo::SurfaceId;

static uint32_t kGLES2BoundSurfaceLocalId = 1u;

namespace native_viewport {

ViewportSurface::ViewportSurface(mojo::SurfacePtr surface,
                                 mojo::Gpu* gpu_service,
                                 const gfx::Size& size,
                                 cc::SurfaceId child_id)
    : surface_(surface.Pass()),
      gpu_service_(gpu_service),
      widget_id_(0u),
      size_(size),
      gles2_bound_surface_created_(false),
      child_id_(child_id),
      weak_factory_(this) {
}

ViewportSurface::~ViewportSurface() {
}

void ViewportSurface::SetWidgetId(uint64_t widget_id) {
  widget_id_ = widget_id;
  BindSurfaceToNativeViewport();
}

void ViewportSurface::SetSize(const gfx::Size& size) {
  if (size_ == size)
    return;

  size_ = size;
  if (!gles2_bound_surface_created_)
    return;

  surface_->DestroySurface(kGLES2BoundSurfaceLocalId);
  if (widget_id_)
    BindSurfaceToNativeViewport();
}

void ViewportSurface::SetChildId(cc::SurfaceId child_id) {
  child_id_ = child_id;
  SubmitFrame();
}

void ViewportSurface::BindSurfaceToNativeViewport() {
  mojo::ViewportParameterListenerPtr listener;
  auto listener_request = GetProxy(&listener);
  mojo::CommandBufferPtr command_buffer;
  gpu_service_->CreateOnscreenGLES2Context(widget_id_, Size::From(size_),
                                           GetProxy(&command_buffer),
                                           listener.Pass());

  gles2_bound_surface_created_ = true;
  surface_->CreateGLES2BoundSurface(command_buffer.Pass(),
                                    kGLES2BoundSurfaceLocalId,
                                    Size::From(size_), listener_request.Pass());

  SubmitFrame();
}

void ViewportSurface::SubmitFrame() {
  if (child_id_.is_null() || !gles2_bound_surface_created_)
    return;

  auto surface_quad_state = mojo::SurfaceQuadState::New();
  surface_quad_state->surface = SurfaceId::From(child_id_);

  gfx::Rect bounds(size_);

  auto surface_quad = mojo::Quad::New();
  surface_quad->material = mojo::Material::MATERIAL_SURFACE_CONTENT;
  surface_quad->rect = mojo::Rect::From(bounds);
  surface_quad->opaque_rect = mojo::Rect::From(bounds);
  surface_quad->visible_rect = mojo::Rect::From(bounds);
  surface_quad->needs_blending = true;
  surface_quad->shared_quad_state_index = 0;
  surface_quad->surface_quad_state = surface_quad_state.Pass();

  auto pass = CreateDefaultPass(1, *mojo::Rect::From(bounds));

  pass->quads.push_back(surface_quad.Pass());
  pass->shared_quad_states.push_back(CreateDefaultSQS(
      *mojo::Size::From(size_)));

  auto frame = mojo::Frame::New();
  frame->passes.push_back(pass.Pass());
  frame->resources.resize(0u);
  surface_->SubmitFrame(kGLES2BoundSurfaceLocalId, frame.Pass(),
                        mojo::Closure());
}

}  // namespace native_viewport
