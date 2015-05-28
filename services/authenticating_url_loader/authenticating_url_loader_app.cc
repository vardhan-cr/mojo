// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/authenticating_url_loader/authenticating_url_loader_app.h"

#include "mojo/public/cpp/application/application_impl.h"
#include "services/authenticating_url_loader/authenticating_url_loader_factory_impl.h"

namespace mojo {

AuthenticatingURLLoaderApp::AuthenticatingURLLoaderApp() {
}

AuthenticatingURLLoaderApp::~AuthenticatingURLLoaderApp() {
}

void AuthenticatingURLLoaderApp::Initialize(ApplicationImpl* app) {
  app_ = app;
}

bool AuthenticatingURLLoaderApp::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  connection->AddService<AuthenticatingURLLoaderFactory>(this);
  return true;
}

void AuthenticatingURLLoaderApp::Create(
    ApplicationConnection* connection,
    InterfaceRequest<AuthenticatingURLLoaderFactory> request) {
  GURL app_url(connection->GetRemoteApplicationURL());
  GURL app_origin;
  if (app_url.is_valid()) {
    app_origin = app_url.GetOrigin();
  }
  new AuthenticatingURLLoaderFactoryImpl(request.Pass(), app_,
                                         &tokens_[app_origin]);
}

}  // namespace mojo
