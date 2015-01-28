// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/surfaces/surfaces_impl.h"

#include "base/debug/trace_event.h"
#include "cc/output/compositor_frame.h"
#include "cc/resources/returned_resource.h"
#include "cc/surfaces/display.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "mojo/cc/context_provider_mojo.h"
#include "mojo/cc/direct_output_surface.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"

using mojo::SurfaceIdPtr;

namespace surfaces {

namespace {
void CallCallback(const mojo::Closure& callback, cc::SurfaceDrawStatus status) {
  callback.Run();
}
}

SurfacesImpl::SurfacesImpl(cc::SurfaceManager* manager,
                           uint32_t id_namespace,
                           Client* client,
                           mojo::InterfaceRequest<mojo::Surface> request)
    : SurfacesImpl(manager, id_namespace, client) {
  binding_.Bind(request.Pass());
  binding_.client()->SetIdNamespace(id_namespace);
}

SurfacesImpl::SurfacesImpl(cc::SurfaceManager* manager,
                           uint32_t id_namespace,
                           Client* client,
                           mojo::SurfacePtr* surface)
    : SurfacesImpl(manager, id_namespace, client) {
  binding_.Bind(surface);
  binding_.client()->SetIdNamespace(id_namespace);
}

SurfacesImpl::~SurfacesImpl() {
  client_->OnDisplayBeingDestroyed(display_.get());
  factory_.DestroyAll();
}

void SurfacesImpl::CreateSurface(uint32_t local_id) {
  factory_.Create(QualifyIdentifier(local_id));
}

void SurfacesImpl::SubmitFrame(uint32_t local_id,
                               mojo::FramePtr frame,
                               const mojo::Closure& callback) {
  TRACE_EVENT0("mojo", "SurfacesImpl::SubmitFrame");
  factory_.SubmitFrame(QualifyIdentifier(local_id),
                       frame.To<scoped_ptr<cc::CompositorFrame>>(),
                       base::Bind(&CallCallback, callback));
  client_->FrameSubmitted();
}

void SurfacesImpl::DestroySurface(uint32_t local_id) {
  factory_.Destroy(QualifyIdentifier(local_id));
}

void SurfacesImpl::CreateGLES2BoundSurface(
    mojo::CommandBufferPtr gles2_client,
    uint32_t local_id,
    mojo::SizePtr size,
    mojo::InterfaceRequest<mojo::ViewportParameterListener> listener_request) {
  command_buffer_handle_ = gles2_client.PassMessagePipe();

  if (!display_) {
    cc::RendererSettings settings;
    display_.reset(new cc::Display(this, manager_, nullptr, nullptr, settings));
    client_->SetDisplay(display_.get());
    display_->Initialize(make_scoped_ptr(new mojo::DirectOutputSurface(
        new mojo::ContextProviderMojo(command_buffer_handle_.Pass()))));
  }
  cc::SurfaceId cc_id = QualifyIdentifier(local_id);
  factory_.Create(cc_id);
  display_->SetSurfaceId(cc_id, 1.f);
  display_->Resize(size.To<gfx::Size>());
  parameter_listeners_.AddBinding(this, listener_request.Pass());
}

void SurfacesImpl::ReturnResources(const cc::ReturnedResourceArray& resources) {
  if (resources.empty())
    return;
  mojo::Array<mojo::ReturnedResourcePtr> ret(resources.size());
  for (size_t i = 0; i < resources.size(); ++i) {
    ret[i] = mojo::ReturnedResource::From(resources[i]);
  }
  binding_.client()->ReturnResources(ret.Pass());
}

void SurfacesImpl::DisplayDamaged() {
}

void SurfacesImpl::DidSwapBuffers() {
}

void SurfacesImpl::DidSwapBuffersComplete() {
}

void SurfacesImpl::CommitVSyncParameters(base::TimeTicks timebase,
                                         base::TimeDelta interval) {
}

void SurfacesImpl::OutputSurfaceLost() {
}

void SurfacesImpl::SetMemoryPolicy(const cc::ManagedMemoryPolicy& policy) {
}

void SurfacesImpl::OnVSyncParametersUpdated(int64_t timebase,
                                            int64_t interval) {
  client_->OnVSyncParametersUpdated(
      base::TimeTicks::FromInternalValue(timebase),
      base::TimeDelta::FromInternalValue(interval));
}

SurfacesImpl::SurfacesImpl(cc::SurfaceManager* manager,
                           uint32_t id_namespace,
                           Client* client)
    : manager_(manager),
      factory_(manager, this),
      id_namespace_(id_namespace),
      client_(client),
      binding_(this) {
}

cc::SurfaceId SurfacesImpl::QualifyIdentifier(uint32_t local_id) {
  return cc::SurfaceId(static_cast<uint64_t>(id_namespace_) << 32 | local_id);
}

}  // namespace mojo
