// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/display_manager.h"

#include "base/numerics/safe_conversions.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/surfaces/public/cpp/surfaces_utils.h"
#include "mojo/services/surfaces/public/interfaces/quads.mojom.h"
#include "services/view_manager/connection_manager.h"
#include "services/view_manager/server_view.h"
#include "services/view_manager/view_coordinate_conversions.h"

using mojo::Rect;
using mojo::Size;

namespace view_manager {
namespace {

void DrawViewTree(mojo::Pass* pass,
                  const ServerView* view,
                  const gfx::Vector2d& parent_to_root_origin_offset,
                  float opacity) {
  if (!view->visible())
    return;

  const gfx::Rect absolute_bounds =
      view->bounds() + parent_to_root_origin_offset;
  std::vector<const ServerView*> children(view->GetChildren());
  const float combined_opacity = opacity * view->opacity();
  for (std::vector<const ServerView*>::reverse_iterator it = children.rbegin();
       it != children.rend();
       ++it) {
    DrawViewTree(pass, *it, absolute_bounds.OffsetFromOrigin(),
                 combined_opacity);
  }

  cc::SurfaceId node_id = view->surface_id();

  auto surface_quad_state = mojo::SurfaceQuadState::New();
  surface_quad_state->surface = mojo::SurfaceId::From(node_id);

  gfx::Transform node_transform;
  node_transform.Translate(absolute_bounds.x(), absolute_bounds.y());

  const gfx::Rect bounds_at_origin(view->bounds().size());
  auto surface_quad = mojo::Quad::New();
  surface_quad->material = mojo::Material::MATERIAL_SURFACE_CONTENT;
  surface_quad->rect = Rect::From(bounds_at_origin);
  surface_quad->opaque_rect = Rect::From(bounds_at_origin);
  surface_quad->visible_rect = Rect::From(bounds_at_origin);
  surface_quad->needs_blending = true;
  surface_quad->shared_quad_state_index =
      base::saturated_cast<int32_t>(pass->shared_quad_states.size());
  surface_quad->surface_quad_state = surface_quad_state.Pass();

  auto sqs = CreateDefaultSQS(*Size::From(view->bounds().size()));
  sqs->blend_mode = mojo::SK_XFERMODE_kSrcOver_Mode;
  sqs->opacity = combined_opacity;
  sqs->content_to_target_transform = mojo::Transform::From(node_transform);

  pass->quads.push_back(surface_quad.Pass());
  pass->shared_quad_states.push_back(sqs.Pass());
}

}  // namespace

DefaultDisplayManager::DefaultDisplayManager(
    mojo::ApplicationConnection* app_connection,
    const mojo::Callback<void()>& native_viewport_closed_callback)
    : app_connection_(app_connection),
      connection_manager_(nullptr),
      size_(800, 600),
      draw_timer_(false, false),
      native_viewport_closed_callback_(native_viewport_closed_callback),
      weak_factory_(this) {
}

void DefaultDisplayManager::Init(ConnectionManager* connection_manager) {
  connection_manager_ = connection_manager;
  app_connection_->ConnectToService("mojo:native_viewport_service",
                                    &native_viewport_);
  native_viewport_.set_client(this);
  native_viewport_->Create(
      Size::From(size_),
      base::Bind(&DefaultDisplayManager::OnCreatedNativeViewport,
                 weak_factory_.GetWeakPtr()));
  native_viewport_->Show();
  app_connection_->ConnectToService("mojo:surfaces_service",
                                    &surfaces_service_);
  surfaces_service_->CreateSurfaceConnection(
      base::Bind(&DefaultDisplayManager::OnSurfaceConnectionCreated,
                 weak_factory_.GetWeakPtr()));

  mojo::NativeViewportEventDispatcherPtr event_dispatcher;
  app_connection_->ConnectToService(&event_dispatcher);
  native_viewport_->SetEventDispatcher(event_dispatcher.Pass());
}

DefaultDisplayManager::~DefaultDisplayManager() {
}

void DefaultDisplayManager::SchedulePaint(const ServerView* view,
                                          const gfx::Rect& bounds) {
  if (!view->IsDrawn(connection_manager_->root()))
    return;
  const gfx::Rect root_relative_rect =
      ConvertRectBetweenViews(view, connection_manager_->root(), bounds);
  if (root_relative_rect.IsEmpty())
    return;
  dirty_rect_.Union(root_relative_rect);
  if (!draw_timer_.IsRunning()) {
    draw_timer_.Start(
        FROM_HERE, base::TimeDelta(),
        base::Bind(&DefaultDisplayManager::Draw, base::Unretained(this)));
  }
}

void DefaultDisplayManager::SetViewportSize(const gfx::Size& size) {
  native_viewport_->SetSize(Size::From(size));
}

void DefaultDisplayManager::OnCreatedNativeViewport(
    uint64_t native_viewport_id) {
}

void DefaultDisplayManager::OnSurfaceConnectionCreated(mojo::SurfacePtr surface,
                                                       uint32_t id_namespace) {
  surface_ = surface.Pass();
  surface_.set_client(this);
  surface_id_allocator_.reset(new cc::SurfaceIdAllocator(id_namespace));
  Draw();
}

void DefaultDisplayManager::Draw() {
  if (!surface_)
    return;
  if (surface_id_.is_null()) {
    surface_id_ = surface_id_allocator_->GenerateId();
    surface_->CreateSurface(mojo::SurfaceId::From(surface_id_));
  }

  Rect rect;
  rect.width = size_.width();
  rect.height = size_.height();
  auto pass = CreateDefaultPass(1, rect);
  pass->damage_rect = Rect::From(dirty_rect_);

  DrawViewTree(pass.get(), connection_manager_->root(), gfx::Vector2d(), 1.0f);

  auto frame = mojo::Frame::New();
  frame->passes.push_back(pass.Pass());
  frame->resources.resize(0u);
  surface_->SubmitFrame(mojo::SurfaceId::From(surface_id_), frame.Pass(),
                        mojo::Closure());

  native_viewport_->SubmittedFrame(mojo::SurfaceId::From(surface_id_));

  dirty_rect_ = gfx::Rect();
}

void DefaultDisplayManager::OnDestroyed() {
  // This is called when the native_viewport is torn down before
  // ~DefaultDisplayManager may be called.
  native_viewport_closed_callback_.Run();
}

void DefaultDisplayManager::OnMetricsChanged(mojo::ViewportMetricsPtr metrics) {
  size_ = metrics->size.To<gfx::Size>();
  connection_manager_->root()->SetBounds(gfx::Rect(size_));
  if (surface_id_.is_null())
    return;
  surface_->DestroySurface(mojo::SurfaceId::From(surface_id_));
  surface_id_ = cc::SurfaceId();
  SchedulePaint(connection_manager_->root(), gfx::Rect(size_));
}

void DefaultDisplayManager::SetIdNamespace(uint32_t id_namespace) {
}

void DefaultDisplayManager::ReturnResources(
    mojo::Array<mojo::ReturnedResourcePtr> resources) {
  DCHECK_EQ(0u, resources.size());
}

}  // namespace view_manager
