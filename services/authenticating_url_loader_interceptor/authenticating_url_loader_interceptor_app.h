// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/authenticating_url_loader_interceptor/public/interfaces/authenticating_url_loader_interceptor_meta_factory.mojom.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_meta_factory_impl.h"

namespace mojo {

class AuthenticatingURLLoaderInterceptorApp
    : public ApplicationDelegate,
      public InterfaceFactory<AuthenticatingURLLoaderInterceptorMetaFactory> {
 public:
  AuthenticatingURLLoaderInterceptorApp();
  ~AuthenticatingURLLoaderInterceptorApp() override;

 private:
  // ApplicationDelegate
  void Initialize(ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override;

  // InterfaceFactory<AuthenticatingURLLoaderInterceptorMetaFactory>
  void Create(ApplicationConnection* connection,
              InterfaceRequest<AuthenticatingURLLoaderInterceptorMetaFactory>
                  request) override;

  ApplicationImpl* app_;
  // Cache received tokens per origin of the connecting app and origin of the
  // loaded URL so that once a token has been requested it is not necessary to
  // do nultiple http connections to retrieve additional resources on the same
  // host.
  std::map<GURL, std::map<GURL, std::string>> tokens_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatingURLLoaderInterceptorApp);
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_INTERCEPTOR_AUTHENTICATING_URL_LOADER_INTERCEPTOR_APP_H_
