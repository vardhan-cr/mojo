// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/weak_binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/authentication/public/interfaces/authentication.mojom.h"

namespace authentication {
namespace {

class DummyAuthenticationApplication
    : public mojo::ApplicationDelegate,
      public mojo::InterfaceFactory<AuthenticationService>,
      public AuthenticationService {
 public:
  DummyAuthenticationApplication() {}
  ~DummyAuthenticationApplication() override {}

 private:
  // ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override{};
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<AuthenticationService>(this);
    return true;
  }

  // InterfaceFactory<AuthenticationService> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<AuthenticationService> request) override {
    bindings_.AddBinding(this, request.Pass());
  }

  // AuthenticationService implementation
  void SelectAccount(bool return_last_selected,
                     const SelectAccountCallback& callback) override {
    callback.Run(nullptr, "Not implemented");
  }
  void GetOAuth2Token(const mojo::String& username,
                      mojo::Array<mojo::String> scopes,
                      const GetOAuth2TokenCallback& callback) override {
    callback.Run(nullptr, "Not implemented");
  }
  void ClearOAuth2Token(const mojo::String& token) override {}

  mojo::WeakBindingSet<AuthenticationService> bindings_;
};

}  // namespace
}  // namespace authentication

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(
      new authentication::DummyAuthenticationApplication);
  return runner.Run(application_request);
}
