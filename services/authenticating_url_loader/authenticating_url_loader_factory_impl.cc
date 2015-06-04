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
    mojo::ApplicationImpl* app,
    std::map<GURL, std::string>* cached_tokens)
    : binding_(this, request.Pass()), app_(app), cached_tokens_(cached_tokens) {
  app_->ConnectToService("mojo:network_service", &network_service_);
}

AuthenticatingURLLoaderFactoryImpl::~AuthenticatingURLLoaderFactoryImpl() {
}

std::string AuthenticatingURLLoaderFactoryImpl::GetCachedToken(
    const GURL& url) {
  GURL origin = url.GetOrigin();
  if (cached_tokens_->find(origin) != cached_tokens_->end()) {
    return (*cached_tokens_)[origin];
  }
  return "";
}

void AuthenticatingURLLoaderFactoryImpl::RetrieveToken(
    const GURL& url,
    const base::Callback<void(std::string)>& callback) {
  if (!authentication_service_) {
    callback.Run("");
    return;
  }
  GURL origin = url.GetOrigin();
  if (pendings_retrieve_token_.find(origin) == pendings_retrieve_token_.end()) {
    if (cached_tokens_->find(origin) != cached_tokens_->end()) {
      // Clear the cached token in case the request is due to that token being
      // stale.
      authentication_service_->ClearOAuth2Token((*cached_tokens_)[origin]);
      cached_tokens_->erase(origin);
    }
    if (cached_accounts_.find(origin) != cached_accounts_.end()) {
      OnAccountSelected(origin, cached_accounts_[origin], mojo::String());
      return;
    }
    authentication_service_->SelectAccount(
        true, base::Bind(&AuthenticatingURLLoaderFactoryImpl::OnAccountSelected,
                         base::Unretained(this), origin));
  }
  pendings_retrieve_token_[origin].push_back(callback);
}

void AuthenticatingURLLoaderFactoryImpl::OnURLLoaderError(
    AuthenticatingURLLoaderImpl* url_loader) {
  auto it = std::find_if(
      url_loaders_.begin(), url_loaders_.end(),
      [url_loader](const std::unique_ptr<AuthenticatingURLLoaderImpl>& p) {
        return p.get() == url_loader;
      });
  DCHECK(it != url_loaders_.end());
  url_loaders_.erase(it);
}

void AuthenticatingURLLoaderFactoryImpl::CreateAuthenticatingURLLoader(
    mojo::InterfaceRequest<AuthenticatingURLLoader> loader_request) {
  url_loaders_.push_back(std::unique_ptr<AuthenticatingURLLoaderImpl>(
      new AuthenticatingURLLoaderImpl(loader_request.Pass(), this)));
}

void AuthenticatingURLLoaderFactoryImpl::SetAuthenticationService(
    authentication::AuthenticationServicePtr authentication_service) {
  // If the authentication service changes, all pending request needs to fail.
  for (const auto& callbacks : pendings_retrieve_token_) {
    for (const auto& callback : callbacks.second) {
      callback.Run("");
    }
  }
  pendings_retrieve_token_.clear();
  authentication_service_ = authentication_service.Pass();
  if (authentication_service_)
    authentication_service_.set_error_handler(this);
}

void AuthenticatingURLLoaderFactoryImpl::OnConnectionError() {
  SetAuthenticationService(nullptr);
}

void AuthenticatingURLLoaderFactoryImpl::OnAccountSelected(const GURL& origin,
                                                           mojo::String account,
                                                           mojo::String error) {
  DCHECK(authentication_service_);
  if (error) {
    LOG(WARNING) << "Error (" << error << ") while selecting account";
    ExecuteCallbacks(origin, "");
    return;
  }
  cached_accounts_[origin] = account;
  mojo::Array<mojo::String> scopes(1);
  scopes[0] = "https://www.googleapis.com/auth/userinfo.email";
  authentication_service_->GetOAuth2Token(
      account, scopes.Pass(),
      base::Bind(&AuthenticatingURLLoaderFactoryImpl::OnOAuth2TokenReceived,
                 base::Unretained(this), origin));
}

void AuthenticatingURLLoaderFactoryImpl::OnOAuth2TokenReceived(
    const GURL& origin,
    mojo::String token,
    mojo::String error) {
  if (error) {
    LOG(WARNING) << "Error (" << error << ") while getting token";
    ExecuteCallbacks(origin, "");
    return;
  }
  std::string string_token(token);
  (*cached_tokens_)[origin] = string_token;
  ExecuteCallbacks(origin, string_token);
}

void AuthenticatingURLLoaderFactoryImpl::ExecuteCallbacks(
    const GURL& origin,
    const std::string& result) {
  for (auto& callback : pendings_retrieve_token_[origin]) {
    callback.Run(result);
  }
  pendings_retrieve_token_.erase(origin);
}

}  // namespace mojo
