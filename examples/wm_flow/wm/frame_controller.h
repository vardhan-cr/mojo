// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_WM_FLOW_WM_FRAME_CONTROLLER_H_
#define EXAMPLES_WM_FLOW_WM_FRAME_CONTROLLER_H_

#include "base/memory/scoped_ptr.h"
#include "examples/wm_flow/wm/window_frame_host.mojom.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "services/window_manager/focus_controller.h"
#include "ui/gfx/geometry/rect.h"

class GURL;

namespace mojo {
class NativeWidgetViewManager;
class View;
}

namespace window_manager {
class WindowManagerRoot;
}

// FrameController encapsulates the window manager's frame additions to a window
// created by an application. It renders the content of the frame and responds
// to any events targeted at it.
class FrameController
    : public examples::WindowFrameHost,
      public mojo::ViewObserver,
      public mojo::InterfaceFactory<examples::WindowFrameHost> {
 public:
  FrameController(const GURL& frame_app_url,
                  mojo::View* view,
                  mojo::View** app_view,
                  window_manager::WindowManagerRoot* window_manager_root);
  ~FrameController() override;

  // mojo::InterfaceFactory<examples::WindowFrameHost> implementation.
  void Create(
      mojo::ApplicationConnection* connection,
      mojo::InterfaceRequest<examples::WindowFrameHost> request) override;

  // examples::WindowFrameHost
  void CloseWindow() override;
  void ToggleMaximize() override;
  void ActivateWindow() override;
  void SetCapture(bool frame_has_capture) override;

 private:
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;

  mojo::View* view_;
  mojo::View* app_view_;
  bool maximized_;
  gfx::Rect restored_bounds_;
  window_manager::WindowManagerRoot* window_manager_root_;
  mojo::ServiceProviderImpl viewer_services_impl_;

  mojo::Binding<examples::WindowFrameHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(FrameController);
};

#endif  // EXAMPLES_WM_FLOW_WM_FRAME_CONTROLLER_H_
