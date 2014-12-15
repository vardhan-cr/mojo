// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/fake_surfaces/fake_surfaces_service_application.h"

#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/services/public/interfaces/surfaces/surfaces.mojom.h"

using mojo::InterfaceRequest;

namespace fake_surfaces {

class FakeSurfaceImpl : public mojo::Surface {
 public:
  FakeSurfaceImpl(uint32_t id_namespace, mojo::SurfacePtr* ptr)
      : binding_(this, ptr) {}
  ~FakeSurfaceImpl() override {}

  // mojo::Surface implementation.
  void CreateSurface(mojo::SurfaceIdPtr id, mojo::SizePtr size) override {}

  void SubmitFrame(mojo::SurfaceIdPtr id,
                   mojo::FramePtr frame,
                   const mojo::Closure& callback) override {
    callback.Run();
    if (frame->resources.size() == 0u)
      return;
    mojo::Array<mojo::ReturnedResourcePtr> returned;
    returned.resize(frame->resources.size());
    for (size_t i = 0; i < frame->resources.size(); ++i) {
      auto ret = mojo::ReturnedResource::New();
      ret->id = frame->resources[i]->id;
      ret->sync_point = 0u;
      ret->count = 1;
      ret->lost = false;
      returned[i] = ret.Pass();
    }
    binding_.client()->ReturnResources(returned.Pass());
  }

  void DestroySurface(mojo::SurfaceIdPtr id) override {}

  void CreateGLES2BoundSurface(
      mojo::CommandBufferPtr gles2_client,
      mojo::SurfaceIdPtr id,
      mojo::SizePtr size,
      mojo::InterfaceRequest<mojo::ViewportParameterListener> listener_request)
      override {
  }

 private:
  mojo::StrongBinding<mojo::Surface> binding_;

  DISALLOW_COPY_AND_ASSIGN(FakeSurfaceImpl);
};

class FakeSurfacesServiceImpl : public mojo::SurfacesService {
 public:
  FakeSurfacesServiceImpl(uint32_t* id_namespace,
                          InterfaceRequest<mojo::SurfacesService> request)
      : binding_(this, request.Pass()), next_id_namespace_(id_namespace) {}
  ~FakeSurfacesServiceImpl() override {}

  // mojo::SurfacesService implementation.
  void CreateSurfaceConnection(
      const mojo::Callback<void(mojo::SurfacePtr, uint32_t)>& callback)
      override {
    mojo::SurfacePtr surface;
    uint32_t id_namespace = (*next_id_namespace_)++;
    new FakeSurfaceImpl(id_namespace, &surface);
    callback.Run(surface.Pass(), id_namespace);
  }

 private:
  mojo::StrongBinding<mojo::SurfacesService> binding_;
  uint32_t* next_id_namespace_;

  DISALLOW_COPY_AND_ASSIGN(FakeSurfacesServiceImpl);
};

FakeSurfacesServiceApplication::FakeSurfacesServiceApplication()
    : next_id_namespace_(1u) {
}

FakeSurfacesServiceApplication::~FakeSurfacesServiceApplication() {
}

void FakeSurfacesServiceApplication::Initialize(mojo::ApplicationImpl* app) {
  mojo::TracingImpl::Create(app);
}

bool FakeSurfacesServiceApplication::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(this);
  return true;
}

void FakeSurfacesServiceApplication::Create(
    mojo::ApplicationConnection* connection,
    InterfaceRequest<mojo::SurfacesService> request) {
  new FakeSurfacesServiceImpl(&next_id_namespace_, request.Pass());
}

}  // namespace fake_surfaces

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new fake_surfaces::FakeSurfacesServiceApplication);
  return runner.Run(shell_handle);
}
