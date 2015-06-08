// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_meta_factory_impl.h"

#include "mojo/public/cpp/application/application_impl.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_factory.h"

namespace mojo {

AuthenticatingURLLoaderInterceptorMetaFactoryImpl::
    AuthenticatingURLLoaderInterceptorMetaFactoryImpl(
        mojo::InterfaceRequest<AuthenticatingURLLoaderInterceptorMetaFactory>
            request,
        mojo::ApplicationImpl* app,
        std::map<GURL, std::string>* cached_tokens)
    : binding_(this, request.Pass()), app_(app), cached_tokens_(cached_tokens) {
}

AuthenticatingURLLoaderInterceptorMetaFactoryImpl::
    ~AuthenticatingURLLoaderInterceptorMetaFactoryImpl() {
}

void AuthenticatingURLLoaderInterceptorMetaFactoryImpl::
    CreateURLLoaderInterceptorFactory(
        mojo::InterfaceRequest<URLLoaderInterceptorFactory> factory_request,
        authentication::AuthenticationServicePtr authentication_service) {
  new AuthenticatingURLLoaderInterceptorFactory(factory_request.Pass(),
                                                authentication_service.Pass(),
                                                app_, cached_tokens_);
}

}  // namespace mojo
