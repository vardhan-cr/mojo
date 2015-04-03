// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "examples/surfaces_app/child_impl.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/bindings/string.h"

namespace mojo {
namespace examples {

class ChildApp : public ApplicationDelegate, public InterfaceFactory<Child> {
 public:
  ChildApp() {}
  ~ChildApp() override {}

  void Initialize(ApplicationImpl* app) override {
    surfaces_service_connection_ =
        app->ConnectToApplication("mojo:surfaces_service");
  }

  // ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

  // InterfaceFactory<Child> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<Child> request) override {
    new ChildImpl(surfaces_service_connection_, request.Pass());
  }

 private:
  ApplicationConnection* surfaces_service_connection_;

  DISALLOW_COPY_AND_ASSIGN(ChildApp);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::ChildApp);
  return runner.Run(application_request);
}
