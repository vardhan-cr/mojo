// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/view_manager_app.h"

#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/view_manager/view_manager_root_connection.h"

using mojo::ApplicationConnection;
using mojo::ApplicationImpl;
using mojo::InterfaceRequest;
using mojo::ViewManagerService;
using mojo::WindowManagerInternalClient;

namespace view_manager {

ViewManagerApp::ViewManagerApp() : app_impl_(nullptr) {}

ViewManagerApp::~ViewManagerApp() {}

void ViewManagerApp::Initialize(ApplicationImpl* app) {
  app_impl_ = app;
  tracing_.Initialize(app);
}

bool ViewManagerApp::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  // We keep a raw pointer in active_root_connections_ in order to be able to
  // close the ViewManagerApp when all outstanding ViewManagerRootConnections
  // are closed. ViewManagerRootConnection manages its own lifetime.
  ViewManagerRootConnection* root_connection =
      new ViewManagerRootConnection(app_impl_, this);
  if (root_connection->Init(connection)) {
    active_root_connections_.insert(root_connection);
    return true;
  } else {
    return false;
  }
}

void ViewManagerApp::OnCloseViewManagerRootConnection(
    ViewManagerRootConnection* view_manager_root_connection) {
  active_root_connections_.erase(view_manager_root_connection);

  if (active_root_connections_.size() == 0) {
    ApplicationImpl::Terminate();
  }
}

}  // namespace view_manager
