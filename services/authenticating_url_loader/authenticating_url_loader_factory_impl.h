// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/authenticating_url_loader/public/interfaces/authenticating_url_loader_factory.mojom.h"
#include "mojo/services/authentication/public/interfaces/authentication.mojom.h"
#include "url/gurl.h"

namespace mojo {

class ApplicationImpl;

class AuthenticatingURLLoaderFactoryImpl
    : public AuthenticatingURLLoaderFactory {
 public:
  AuthenticatingURLLoaderFactoryImpl(
      mojo::InterfaceRequest<AuthenticatingURLLoaderFactory> request,
      mojo::ApplicationImpl* app,
      std::map<GURL, std::string>* cached_tokens);
  ~AuthenticatingURLLoaderFactoryImpl() override;

 private:
  // AuthenticatingURLLoaderFactory:
  void CreateURLLoaderInterceptorFactory(
      mojo::InterfaceRequest<URLLoaderInterceptorFactory> factory_request,
      authentication::AuthenticationServicePtr authentication_service) override;

  StrongBinding<AuthenticatingURLLoaderFactory> binding_;
  ApplicationImpl* app_;
  std::map<GURL, std::string>* cached_tokens_;
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_
