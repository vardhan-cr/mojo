// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader/authenticating_url_loader_factory_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/authenticating_url_loader/public/interfaces/authenticating_url_loader.mojom.h"
#include "services/authenticating_url_loader/authenticating_url_loader_impl.h"

namespace mojo {

AuthenticatingURLLoaderFactoryImpl::AuthenticatingURLLoaderFactoryImpl(
    mojo::InterfaceRequest<AuthenticatingURLLoaderFactory> request,
    mojo::ApplicationImpl* app)
    : binding_(this, request.Pass()), app_(app) {
  app_->ConnectToService("mojo:network_service", &network_service_);
}

AuthenticatingURLLoaderFactoryImpl::~AuthenticatingURLLoaderFactoryImpl() {
}

void AuthenticatingURLLoaderFactoryImpl::SetAuthenticationService(
    authentication::AuthenticationServicePtr authentication_service) {
  authentication_service_ = authentication_service.Pass();
  if (authentication_service_)
    authentication_service_.set_error_handler(this);

  for (const auto& url_loader : url_loaders_) {
    url_loader->SetAuthenticationService(authentication_service_.get());
  }
}

void AuthenticatingURLLoaderFactoryImpl::CreateAuthenticatingURLLoader(
    mojo::InterfaceRequest<AuthenticatingURLLoader> loader_request) {
  url_loaders_.push_back(std::unique_ptr<AuthenticatingURLLoaderImpl>(
      new AuthenticatingURLLoaderImpl(
          loader_request.Pass(), authentication_service_.get(),
          network_service_.get(), &cached_tokens_,
          base::Bind(&AuthenticatingURLLoaderFactoryImpl::DeleteURLLoader,
                     base::Unretained(this)))));
}

void AuthenticatingURLLoaderFactoryImpl::OnConnectionError() {
  SetAuthenticationService(nullptr);
}

void AuthenticatingURLLoaderFactoryImpl::DeleteURLLoader(
    AuthenticatingURLLoaderImpl* url_loader) {
  auto it = std::find_if(
      url_loaders_.begin(), url_loaders_.end(),
      [url_loader](const std::unique_ptr<AuthenticatingURLLoaderImpl>& p) {
        return p.get() == url_loader;
      });
  DCHECK(it != url_loaders_.end());
  url_loaders_.erase(it);
}

}  // namespace mojo
