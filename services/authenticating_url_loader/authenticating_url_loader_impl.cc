// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader/authenticating_url_loader_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"

namespace mojo {

AuthenticatingURLLoaderImpl::AuthenticatingURLLoaderImpl(
    InterfaceRequest<AuthenticatingURLLoader> request,
    authentication::AuthenticationService* authentication_service,
    NetworkService* network_service,
    const Callback<void(AuthenticatingURLLoaderImpl*)>&
        connection_error_callback)
    : binding_(this, request.Pass()),
      authentication_service_(authentication_service),
      network_service_(network_service),
      connection_error_callback_(connection_error_callback),
      request_authorization_state_(REQUEST_NOT_AUTHORIZED) {
  binding_.set_error_handler(this);
}

AuthenticatingURLLoaderImpl::~AuthenticatingURLLoaderImpl() {
}

void AuthenticatingURLLoaderImpl::Start(
    URLRequestPtr request,
    const Callback<void(URLResponsePtr)>& callback) {
  // TODO(blundell): If we need to handle requests with bodies, we'll need to
  // do something here.
  if (request->body) {
    LOG(ERROR)
        << "Cannot pass a request to AuthenticatingURLLoader that has a body";
    callback.Run(nullptr);
    return;
  }
  url_ = request->url;
  auto_follow_redirects_ = request->auto_follow_redirects;
  bypass_cache_ = request->bypass_cache;
  headers_ = request->headers.Clone();
  pending_request_callback_ = callback;
  StartNetworkRequest(request.Pass());
}

void AuthenticatingURLLoaderImpl::FollowRedirect(
    const Callback<void(URLResponsePtr)>& callback) {
  mojo::String error;

  if (!url_ || !url_loader_) {
    error = "No redirect to follow";
  }

  if (auto_follow_redirects_) {
    error =
        "FollowRedirect() should not be "
        "called when auto_follow_redirects has been set";
  }

  if (username_ || (request_authorization_state_ != REQUEST_NOT_AUTHORIZED)) {
    error = "Not in the right state to follow a redirect";
  }

  if (!error.is_null()) {
    LOG(ERROR) << "AuthenticatingURLLoader: " << error;
    callback.Run(nullptr);
    return;
  }

  pending_request_callback_ = callback;
  FollowRedirectInternal();
}

void AuthenticatingURLLoaderImpl::StartNetworkRequest(URLRequestPtr request) {
  network_service_->CreateURLLoader(mojo::GetProxy(&url_loader_));
  url_loader_->Start(request.Pass(),
                     base::Bind(&AuthenticatingURLLoaderImpl::OnLoadComplete,
                                base::Unretained(this)));
}

void AuthenticatingURLLoaderImpl::OnConnectionError() {
  connection_error_callback_.Run(this);
  // The callback deleted this object.
}

void AuthenticatingURLLoaderImpl::OnLoadComplete(URLResponsePtr response) {
  if (response->redirect_url) {
    url_ = response->redirect_url;
    username_ = nullptr;
    request_authorization_state_ = REQUEST_NOT_AUTHORIZED;

    if (auto_follow_redirects_) {
      FollowRedirectInternal();
    } else {
      // NOTE: We do not reset |url_loader_| here as it will be needed if the
      // client calls |FollowRedirect()|.
      pending_request_callback_.Run(response.Pass());
    }
    return;
  }

  url_loader_.reset();

  if (response->status_code != 401 || !authentication_service_ ||
      request_authorization_state_ == REQUEST_AUTHORIZED_WITH_FRESH_TOKEN) {
    pending_request_callback_.Run(response.Pass());
    return;
  }

  pending_response_ = response.Pass();

  if (request_authorization_state_ == REQUEST_NOT_AUTHORIZED) {
    DCHECK(!username_);
    request_authorization_state_ = REQUEST_AUTHORIZED_WITH_CACHED_TOKEN;
    authentication_service_->SelectAccount(
        base::Bind(&AuthenticatingURLLoaderImpl::OnAccountSelected,
                   base::Unretained(this)));
    return;
  }

  DCHECK(request_authorization_state_ == REQUEST_AUTHORIZED_WITH_CACHED_TOKEN);
  // Clear the cached token in case the failure was due to that token being
  // stale and try again. If a fresh token doesn't work, we'll have to give up.
  DCHECK(authentication_service_);
  DCHECK(username_);
  authentication_service_->ClearOAuth2Token(token_);
  token_ = String();
  request_authorization_state_ = REQUEST_AUTHORIZED_WITH_FRESH_TOKEN;
  OnAccountSelected(username_, String());
}

void AuthenticatingURLLoaderImpl::FollowRedirectInternal() {
  DCHECK(url_);
  DCHECK(url_loader_);
  DCHECK(!username_);
  DCHECK(request_authorization_state_ == REQUEST_NOT_AUTHORIZED);

  url_loader_->FollowRedirect(base::Bind(
      &AuthenticatingURLLoaderImpl::OnLoadComplete, base::Unretained(this)));
}

void AuthenticatingURLLoaderImpl::OnAccountSelected(String username,
                                                    String error) {
  if (error) {
    LOG(ERROR) << "Error (" << error << ") while selecting account";
    pending_request_callback_.Run(pending_response_.Pass());
    return;
  }

  DCHECK(username);
  username_ = username;

  mojo::Array<mojo::String> scopes(1);
  scopes[0] = "https://www.googleapis.com/auth/userinfo.email";

  authentication_service_->GetOAuth2Token(
      username, scopes.Pass(),
      base::Bind(&AuthenticatingURLLoaderImpl::OnOAuth2TokenReceived,
                 base::Unretained(this)));
}

void AuthenticatingURLLoaderImpl::OnOAuth2TokenReceived(String token,
                                                        String error) {
  if (error) {
    LOG(ERROR) << "Error (" << error << ") while getting token";
    pending_request_callback_.Run(pending_response_.Pass());
    return;
  }

  DCHECK(token);
  token_ = token;
  mojo::Array<mojo::String> headers(0);
  if (headers_)
    headers = headers_.Clone();
  headers.push_back("Authorization: Bearer " + token.get());

  URLRequestPtr request(mojo::URLRequest::New());
  request->url = url_;
  request->auto_follow_redirects = false;
  request->bypass_cache = bypass_cache_;
  request->headers = headers.Pass();

  StartNetworkRequest(request.Pass());
}

void AuthenticatingURLLoaderImpl::SetAuthenticationService(
    authentication::AuthenticationService* authentication_service) {
  authentication_service_ = authentication_service;
  if (authentication_service || !pending_response_)
    return;

  // We need authentication but have no AuthenticationService.
  DCHECK(!pending_request_callback_.is_null());
  pending_request_callback_.Run(pending_response_.Pass());
}

}  // namespace mojo
