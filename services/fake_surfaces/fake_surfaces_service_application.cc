// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/fake_surfaces/fake_surfaces_service_application.h"

#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

using mojo::InterfaceRequest;

namespace fake_surfaces {

class FakeSurfaceImpl : public mojo::Surface {
 public:
  FakeSurfaceImpl(uint32_t id_namespace,
                  mojo::InterfaceRequest<mojo::Surface> request)
      : id_namespace_(id_namespace), binding_(this, request.Pass()) {}
  ~FakeSurfaceImpl() override {}

  // mojo::Surface implementation.
  void GetIdNamespace(
      const Surface::GetIdNamespaceCallback& callback) override {
    callback.Run(id_namespace_);
  }

  void SetResourceReturner(mojo::ResourceReturnerPtr returner) override {
    returner_ = returner.Pass();
  }

  void CreateSurface(uint32_t local_id) override {}

  void SubmitFrame(uint32_t local_id,
                   mojo::FramePtr frame,
                   const mojo::Closure& callback) override {
    callback.Run();
    if (frame->resources.size() == 0u || !returner_)
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
    returner_->ReturnResources(returned.Pass());
  }

  void DestroySurface(uint32_t local_id) override {}

  void CreateGLES2BoundSurface(
      mojo::CommandBufferPtr gles2_client,
      uint32_t local_id,
      mojo::SizePtr size,
      mojo::InterfaceRequest<mojo::ViewportParameterListener> listener_request)
      override {}

 private:
  const uint32_t id_namespace_;
  mojo::ResourceReturnerPtr returner_;
  mojo::StrongBinding<mojo::Surface> binding_;

  DISALLOW_COPY_AND_ASSIGN(FakeSurfaceImpl);
};

FakeSurfacesServiceApplication::FakeSurfacesServiceApplication()
    : next_id_namespace_(1u) {
}

FakeSurfacesServiceApplication::~FakeSurfacesServiceApplication() {
}

void FakeSurfacesServiceApplication::Initialize(mojo::ApplicationImpl* app) {
  tracing_.Initialize(app);
}

bool FakeSurfacesServiceApplication::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(this);
  return true;
}

void FakeSurfacesServiceApplication::Create(
    mojo::ApplicationConnection* connection,
    InterfaceRequest<mojo::Surface> request) {
  new FakeSurfaceImpl(next_id_namespace_++, request.Pass());
}

}  // namespace fake_surfaces

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new fake_surfaces::FakeSurfacesServiceApplication);
  return runner.Run(shell_handle);
}
