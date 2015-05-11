// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "services/keyboard_native/keyboard_service_impl.h"

namespace keyboard {

class KeyboardServiceDelegate : public mojo::ApplicationDelegate,
                                public mojo::InterfaceFactory<KeyboardService> {
 public:
  KeyboardServiceDelegate() {}
  ~KeyboardServiceDelegate() override {}

  // ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<KeyboardService>(this);
    return true;
  }

  // InterfaceFactory<KeyboardService> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<KeyboardService> request) override {
    new KeyboardServiceImpl(request.Pass());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceDelegate);
};

}  // namespace keyboard

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new keyboard::KeyboardServiceDelegate());
  return runner.Run(application_request);
}
