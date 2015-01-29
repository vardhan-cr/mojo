// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SURFACES_SURFACES_IMPL_H_
#define SERVICES_SURFACES_SURFACES_IMPL_H_

#include "cc/surfaces/display_client.h"
#include "cc/surfaces/surface_factory.h"
#include "cc/surfaces/surface_factory_client.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/gpu/public/interfaces/command_buffer.mojom.h"
#include "mojo/services/gpu/public/interfaces/viewport_parameter_listener.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace cc {
class Display;
}

namespace mojo {
class ApplicationManager;
}

namespace surfaces {

class SurfacesImpl : public mojo::Surface,
                     public mojo::ViewportParameterListener,
                     public cc::SurfaceFactoryClient,
                     public cc::DisplayClient {
 public:
  class Client {
   public:
    virtual void OnVSyncParametersUpdated(base::TimeTicks timebase,
                                          base::TimeDelta interval) = 0;
    virtual void FrameSubmitted() = 0;
    virtual void SetDisplay(cc::Display* display) = 0;
    virtual void OnDisplayBeingDestroyed(cc::Display* display) = 0;
  };

  SurfacesImpl(cc::SurfaceManager* manager,
               uint32_t id_namespace,
               Client* client,
               mojo::InterfaceRequest<mojo::Surface> request);

  SurfacesImpl(cc::SurfaceManager* manager,
               uint32_t id_namespace,
               Client* client,
               mojo::SurfacePtr* surface);

  ~SurfacesImpl() override;

  // Surface implementation.
  void CreateSurface(uint32_t local_id) override;
  void SubmitFrame(uint32_t local_id,
                   mojo::FramePtr frame,
                   const mojo::Closure& callback) override;
  void DestroySurface(uint32_t local_id) override;
  void CreateGLES2BoundSurface(
      mojo::CommandBufferPtr gles2_client,
      uint32_t local_id,
      mojo::SizePtr size,
      mojo::InterfaceRequest<mojo::ViewportParameterListener> listener_request)
      override;

  // SurfaceFactoryClient implementation.
  void ReturnResources(const cc::ReturnedResourceArray& resources) override;

  // DisplayClient implementation.
  void DisplayDamaged() override;
  void DidSwapBuffers() override;
  void DidSwapBuffersComplete() override;
  void CommitVSyncParameters(base::TimeTicks timebase,
                             base::TimeDelta interval) override;
  void OutputSurfaceLost() override;
  void SetMemoryPolicy(const cc::ManagedMemoryPolicy& policy) override;

  // ViewportParameterListener
  void OnVSyncParametersUpdated(int64_t timebase, int64_t interval) override;

  cc::SurfaceFactory* factory() { return &factory_; }

 private:
  SurfacesImpl(cc::SurfaceManager* manager,
               uint32_t id_namespace,
               Client* client);

  cc::SurfaceId QualifyIdentifier(uint32_t local_id);

  cc::SurfaceManager* manager_;
  cc::SurfaceFactory factory_;
  uint32_t id_namespace_;
  Client* client_;
  scoped_ptr<cc::Display> display_;
  mojo::ScopedMessagePipeHandle command_buffer_handle_;
  mojo::WeakBindingSet<ViewportParameterListener> parameter_listeners_;
  mojo::StrongBinding<Surface> binding_;

  DISALLOW_COPY_AND_ASSIGN(SurfacesImpl);
};

}  // namespace surfaces

#endif  // SERVICES_SURFACES_SURFACES_IMPL_H_
