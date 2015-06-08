// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_app.h"

#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/authenticating_url_loader_interceptor/authenticating_url_loader_interceptor_meta_factory_impl.h"

namespace mojo {

AuthenticatingURLLoaderInterceptorApp::AuthenticatingURLLoaderInterceptorApp() {
}

AuthenticatingURLLoaderInterceptorApp::
    ~AuthenticatingURLLoaderInterceptorApp() {
}

void AuthenticatingURLLoaderInterceptorApp::Initialize(ApplicationImpl* app) {
  app_ = app;
}

bool AuthenticatingURLLoaderInterceptorApp::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  connection->AddService<AuthenticatingURLLoaderInterceptorMetaFactory>(this);
  return true;
}

void AuthenticatingURLLoaderInterceptorApp::Create(
    ApplicationConnection* connection,
    InterfaceRequest<AuthenticatingURLLoaderInterceptorMetaFactory> request) {
  GURL app_url(connection->GetRemoteApplicationURL());
  GURL app_origin;
  if (app_url.is_valid()) {
    app_origin = app_url.GetOrigin();
  }
  new AuthenticatingURLLoaderInterceptorMetaFactoryImpl(request.Pass(), app_,
                                                        &tokens_[app_origin]);
}

}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new mojo::AuthenticatingURLLoaderInterceptorApp);
  return runner.Run(application_request);
}
