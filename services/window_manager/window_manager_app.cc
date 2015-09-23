// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/window_manager/window_manager_app.h"

#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "services/window_manager/capture_controller.h"
#include "services/window_manager/focus_controller.h"
#include "services/window_manager/focus_rules.h"
#include "services/window_manager/hit_test.h"
#include "services/window_manager/view_event_dispatcher.h"
#include "services/window_manager/view_target.h"
#include "services/window_manager/view_targeter.h"
#include "services/window_manager/window_manager_delegate.h"
#include "services/window_manager/window_manager_root.h"

using mojo::ApplicationConnection;
using mojo::Id;
using mojo::ServiceProvider;
using mojo::View;
using mojo::WindowManager;

namespace window_manager {

////////////////////////////////////////////////////////////////////////////////
// WindowManagerApp, public:

WindowManagerApp::WindowManagerApp(
    WindowManagerControllerFactory* controller_factory)
    : app_impl_(nullptr), controller_factory_(controller_factory) {}

WindowManagerApp::~WindowManagerApp() {}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerApp, ApplicationDelegate implementation:

void WindowManagerApp::Initialize(mojo::ApplicationImpl* impl) {
  app_impl_ = impl;
}

bool WindowManagerApp::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<mojo::WindowManager>(this);
  return true;
}

void WindowManagerApp::Create(
    ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::WindowManager> request) {
  // WindowManagerRoot manages its own lifetime.
  new WindowManagerRoot(app_impl_, connection, controller_factory_,
                        request.Pass());
}

}  // namespace window_manager
