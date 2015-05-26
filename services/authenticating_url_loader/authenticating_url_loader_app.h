// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_APP_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_APP_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/authenticating_url_loader/public/interfaces/authenticating_url_loader_factory.mojom.h"
#include "services/authenticating_url_loader/authenticating_url_loader_factory_impl.h"

namespace mojo {

class AuthenticatingURLLoaderApp
    : public ApplicationDelegate,
      public InterfaceFactory<AuthenticatingURLLoaderFactory> {
 public:
  AuthenticatingURLLoaderApp();
  ~AuthenticatingURLLoaderApp() override;

 private:
  // ApplicationDelegate
  void Initialize(ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override;

  // InterfaceFactory<AuthenticatingURLLoaderFactory>
  void Create(
      ApplicationConnection* connection,
      InterfaceRequest<AuthenticatingURLLoaderFactory> request) override;

  ApplicationImpl* app_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatingURLLoaderApp);
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_APP_H_
