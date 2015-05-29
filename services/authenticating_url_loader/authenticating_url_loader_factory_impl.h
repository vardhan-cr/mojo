// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_

#include <memory>

#include "base/callback.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/authenticating_url_loader/public/interfaces/authenticating_url_loader_factory.mojom.h"
#include "mojo/services/authentication/public/interfaces/authentication.mojom.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "url/gurl.h"

namespace mojo {

class ApplicationImpl;

class AuthenticatingURLLoaderImpl;

class AuthenticatingURLLoaderFactoryImpl
    : public AuthenticatingURLLoaderFactory,
      public ErrorHandler {
 public:
  AuthenticatingURLLoaderFactoryImpl(
      mojo::InterfaceRequest<AuthenticatingURLLoaderFactory> request,
      mojo::ApplicationImpl* app,
      std::map<GURL, std::string>* cached_tokens);
  ~AuthenticatingURLLoaderFactoryImpl() override;

  NetworkService* network_service() { return network_service_.get(); }

  // Returns a cached token for the given url (only considers the origin). Will
  // returns an empty string if no token is cached.
  std::string GetCachedToken(const GURL& url);

  void RetrieveToken(const GURL& url,
                     const base::Callback<void(std::string)>& callback);

  void OnURLLoaderError(AuthenticatingURLLoaderImpl* url_loader);

 private:
  // AuthenticatingURLLoaderFactory:
  void CreateAuthenticatingURLLoader(
      mojo::InterfaceRequest<AuthenticatingURLLoader> loader_request) override;
  void SetAuthenticationService(
      authentication::AuthenticationServicePtr authentication_service) override;

  // ErrorHandler:
  void OnConnectionError() override;

  void OnAccountSelected(const GURL& origin,
                         mojo::String account,
                         mojo::String error);

  void OnOAuth2TokenReceived(const GURL& origin,
                             mojo::String token,
                             mojo::String error);

  void ExecuteCallbacks(const GURL& origin, const std::string& result);

  StrongBinding<AuthenticatingURLLoaderFactory> binding_;
  ApplicationImpl* app_;
  std::map<GURL, std::string>* cached_tokens_;
  std::map<GURL, std::string> cached_accounts_;
  authentication::AuthenticationServicePtr authentication_service_;
  NetworkServicePtr network_service_;
  std::vector<std::unique_ptr<AuthenticatingURLLoaderImpl>> url_loaders_;
  std::map<GURL, std::vector<base::Callback<void(std::string)>>>
      pendings_retrieve_token_;
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_FACTORY_IMPL_H_
