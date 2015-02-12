// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/indirect_service/indirect_service_demo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace mojo {
namespace examples {

class IntegerServiceImpl : public IntegerService {
 public:
  explicit IntegerServiceImpl(InterfaceRequest<IntegerService> request)
      : value_(0), binding_(this, request.Pass()) {}
  ~IntegerServiceImpl() override {}

  void Increment(const Callback<void(int32_t)>& callback) override {
    callback.Run(value_++);
  }

 private:
  int32_t value_;
  StrongBinding<IntegerService> binding_;
};

class IntegerServiceAppDelegate : public ApplicationDelegate,
                                  public InterfaceFactory<IntegerService> {
 public:
  bool ConfigureIncomingConnection(
      ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

 private:
  // InterfaceFactory<IntegerService>
  void Create(ApplicationConnection* app,
              InterfaceRequest<IntegerService> request) override {
    new IntegerServiceImpl(request.Pass());
  }
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunner runner(new mojo::examples::IntegerServiceAppDelegate);
  return runner.Run(shell_handle);
}

