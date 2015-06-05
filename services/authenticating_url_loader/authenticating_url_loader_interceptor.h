// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_INTERCEPTOR_H_
#define SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_INTERCEPTOR_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "services/authenticating_url_loader/authenticating_url_loader_interceptor_factory.h"
#include "url/gurl.h"

namespace mojo {

class NetworkService;

enum RequestAuthorizationState {
  REQUEST_INITIAL,
  REQUEST_USED_CURRENT_AUTH_SERVICE_TOKEN,
  REQUEST_USED_FRESH_AUTH_SERVICE_TOKEN,
};

class AuthenticatingURLLoaderInterceptor : public URLLoaderInterceptor,
                                           public ErrorHandler {
 public:
  AuthenticatingURLLoaderInterceptor(
      mojo::InterfaceRequest<URLLoaderInterceptor> request,
      AuthenticatingURLLoaderInterceptorFactory* factory);
  ~AuthenticatingURLLoaderInterceptor() override;

 private:
  // URLLoaderInterceptor:
  void InterceptRequest(mojo::URLRequestPtr request,
                        const InterceptRequestCallback& callback) override;
  void InterceptFollowRedirect(
      const InterceptResponseCallback& callback) override;
  void InterceptResponse(mojo::URLResponsePtr response,
                         const InterceptResponseCallback& callback) override;

  // ErrorHandler:
  void OnConnectionError() override;

  void OnOAuth2TokenReceived(std::string token);

  URLRequestPtr BuildRequest(std::string token);
  void StartRequest(mojo::URLRequestPtr request);

  Binding<URLLoaderInterceptor> binding_;
  AuthenticatingURLLoaderInterceptorFactory* factory_;
  InterceptResponseCallback pending_interception_callback_;
  URLResponsePtr pending_response_;
  RequestAuthorizationState request_authorization_state_;
  GURL url_;
  bool auto_follow_redirects_;
  bool bypass_cache_;
  Array<HttpHeaderPtr> headers_;
};

}  // namespace mojo

#endif  // SERVICES_AUTHENTICATING_URL_LOADER_AUTHENTICATING_URL_LOADER_INTERCEPTOR_H_
