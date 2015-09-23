// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_APP_H_
#define SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_APP_H_

#include "base/macros.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/window_manager/public/interfaces/window_manager.mojom.h"
#include "services/window_manager/window_manager_delegate.h"

namespace window_manager {

// Implements core window manager functionality that could conceivably be shared
// across multiple window managers implementing superficially different user
// experiences.
// A window manager wishing to use this core should create and own an instance
// of this object. They may implement the associated ViewManager/WindowManager
// delegate interfaces exposed by the view manager, this object provides the
// canonical implementation of said interfaces but will call out to the wrapped
// instances.
// Window manager clients should request a WindowManager service to get a new
// native window.
class WindowManagerApp : public mojo::ApplicationDelegate,
                         public mojo::InterfaceFactory<mojo::WindowManager> {
 public:
  WindowManagerApp(WindowManagerControllerFactory* controller_factory);
  ~WindowManagerApp() override;

  // Overridden from ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* impl) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

 private:
  // InterfaceFactory<WindowManager>:
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::WindowManager> request) override;

  mojo::ApplicationImpl* app_impl_;
  WindowManagerControllerFactory* controller_factory_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerApp);
};

}  // namespace window_manager

#endif  // SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_APP_H_
