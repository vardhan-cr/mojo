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
    AuthenticatingURLLoaderFactoryImpl* factory)
    : binding_(this, request.Pass()),
      factory_(factory),
      request_authorization_state_(REQUEST_INITIAL) {
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
  url_ = GURL(request->url);
  auto_follow_redirects_ = request->auto_follow_redirects;
  bypass_cache_ = request->bypass_cache;
  headers_ = request->headers.Clone();
  pending_request_callback_ = callback;
  std::string token = factory_->GetCachedToken(url_);
  if (token != "") {
    auto auth_header = HttpHeader::New();
    auth_header->name = "Authorization";
    auth_header->value = "Bearer " + token;
    request->headers.push_back(auth_header.Pass());
  }
  StartNetworkRequest(request.Pass());
}

void AuthenticatingURLLoaderImpl::FollowRedirect(
    const Callback<void(URLResponsePtr)>& callback) {
  mojo::String error;

  if (!url_.is_valid() || !url_loader_) {
    error = "No redirect to follow";
  }

  if (auto_follow_redirects_) {
    error =
        "FollowRedirect() should not be "
        "called when auto_follow_redirects has been set";
  }

  if (request_authorization_state_ != REQUEST_INITIAL) {
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
  factory_->network_service()->CreateURLLoader(mojo::GetProxy(&url_loader_));
  url_loader_->Start(request.Pass(),
                     base::Bind(&AuthenticatingURLLoaderImpl::OnLoadComplete,
                                base::Unretained(this)));
}

void AuthenticatingURLLoaderImpl::OnConnectionError() {
  factory_->OnURLLoaderError(this);
  // The factory deleted this object.
}

void AuthenticatingURLLoaderImpl::OnLoadComplete(URLResponsePtr response) {
  if (response->redirect_url) {
    url_ = GURL(response->redirect_url);
    request_authorization_state_ = REQUEST_INITIAL;

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

  if (response->status_code != 401 ||
      request_authorization_state_ == REQUEST_USED_FRESH_AUTH_SERVICE_TOKEN) {
    pending_request_callback_.Run(response.Pass());
    return;
  }

  pending_response_ = response.Pass();

  DCHECK(request_authorization_state_ == REQUEST_INITIAL ||
         request_authorization_state_ ==
             REQUEST_USED_CURRENT_AUTH_SERVICE_TOKEN);
  if (request_authorization_state_ == REQUEST_INITIAL) {
    request_authorization_state_ = REQUEST_USED_CURRENT_AUTH_SERVICE_TOKEN;
  } else {
    request_authorization_state_ = REQUEST_USED_FRESH_AUTH_SERVICE_TOKEN;
  }
  factory_->RetrieveToken(
      url_, base::Bind(&AuthenticatingURLLoaderImpl::OnOAuth2TokenReceived,
                       base::Unretained(this)));
  return;
}

void AuthenticatingURLLoaderImpl::FollowRedirectInternal() {
  DCHECK(url_.is_valid());
  DCHECK(url_loader_);
  DCHECK(request_authorization_state_ == REQUEST_INITIAL);

  url_loader_->FollowRedirect(base::Bind(
      &AuthenticatingURLLoaderImpl::OnLoadComplete, base::Unretained(this)));
}

void AuthenticatingURLLoaderImpl::OnOAuth2TokenReceived(std::string token) {
  if (token.empty()) {
    LOG(ERROR) << "Error while getting token";
    pending_request_callback_.Run(pending_response_.Pass());
    return;
  }

  auto auth_header = HttpHeader::New();
  auth_header->name = "Authorization";
  auth_header->value = "Bearer " + token;
  Array<HttpHeaderPtr> headers;
  if (headers_)
    headers = headers_.Clone();
  headers.push_back(auth_header.Pass());

  URLRequestPtr request(mojo::URLRequest::New());
  request->url = url_.spec();
  request->auto_follow_redirects = false;
  request->bypass_cache = bypass_cache_;
  request->headers = headers.Pass();

  StartNetworkRequest(request.Pass());
}

}  // namespace mojo
