// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/fake_surfaces/fake_surfaces_service_application.h"

#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

using mojo::ApplicationConnection;
using mojo::Display;
using mojo::DisplayFactory;
using mojo::InterfaceRequest;
using mojo::ResourceReturnerPtr;
using mojo::StrongBinding;
using mojo::Surface;

namespace fake_surfaces {

namespace {
void ReturnAll(const mojo::Array<mojo::TransferableResourcePtr>& resources,
               mojo::ResourceReturner* returner) {
  mojo::Array<mojo::ReturnedResourcePtr> returned;
  returned.resize(resources.size());
  for (size_t i = 0; i < resources.size(); ++i) {
    auto ret = mojo::ReturnedResource::New();
    ret->id = resources[i]->id;
    ret->sync_point = 0u;
    ret->count = 1;
    ret->lost = false;
    returned[i] = ret.Pass();
  }
  returner->ReturnResources(returned.Pass());
}

}  // namespace

class FakeDisplayImpl : public Display {
 public:
  FakeDisplayImpl(ResourceReturnerPtr returner,
                  InterfaceRequest<Display> request)
      : returner_(returner.Pass()), binding_(this, request.Pass()) {}
  ~FakeDisplayImpl() override {}

 private:
  // Display implementation
  void SubmitFrame(mojo::FramePtr frame,
                   const SubmitFrameCallback& callback) override {
    callback.Run();
    if (frame->resources.size() == 0u || !returner_)
      return;
    ReturnAll(frame->resources, returner_.get());
  }

  ResourceReturnerPtr returner_;
  StrongBinding<Display> binding_;
};

class FakeDisplayFactoryImpl : public DisplayFactory {
 public:
  explicit FakeDisplayFactoryImpl(InterfaceRequest<DisplayFactory> request)
      : binding_(this, request.Pass()) {}
  ~FakeDisplayFactoryImpl() override {}

 private:
  // DisplayFactory implementation.
  void Create(mojo::ContextProviderPtr context_provider,
              ResourceReturnerPtr returner,
              InterfaceRequest<Display> request) override {
    new FakeDisplayImpl(returner.Pass(), request.Pass());
  }

  StrongBinding<DisplayFactory> binding_;
};

class FakeSurfaceImpl : public Surface {
 public:
  FakeSurfaceImpl(uint32_t id_namespace, InterfaceRequest<Surface> request)
      : id_namespace_(id_namespace), binding_(this, request.Pass()) {}
  ~FakeSurfaceImpl() override {}

  // Surface implementation.
  void GetIdNamespace(
      const Surface::GetIdNamespaceCallback& callback) override {
    callback.Run(id_namespace_);
  }

  void SetResourceReturner(ResourceReturnerPtr returner) override {
    returner_ = returner.Pass();
  }

  void CreateSurface(uint32_t local_id) override {}

  void SubmitFrame(uint32_t local_id,
                   mojo::FramePtr frame,
                   const SubmitFrameCallback& callback) override {
    callback.Run();
    if (frame->resources.size() == 0u || !returner_)
      return;
    ReturnAll(frame->resources, returner_.get());
  }

  void DestroySurface(uint32_t local_id) override {}

 private:
  const uint32_t id_namespace_;
  ResourceReturnerPtr returner_;
  StrongBinding<Surface> binding_;

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
    ApplicationConnection* connection) {
  connection->AddService<DisplayFactory>(this);
  connection->AddService<Surface>(this);
  return true;
}

void FakeSurfacesServiceApplication::Create(
    ApplicationConnection* connection,
    InterfaceRequest<DisplayFactory> request) {
  new FakeDisplayFactoryImpl(request.Pass());
}

void FakeSurfacesServiceApplication::Create(ApplicationConnection* connection,
                                            InterfaceRequest<Surface> request) {
  new FakeSurfaceImpl(next_id_namespace_++, request.Pass());
}

}  // namespace fake_surfaces

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new fake_surfaces::FakeSurfacesServiceApplication);
  return runner.Run(shell_handle);
}
