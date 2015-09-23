// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "services/window_manager/basic_focus_rules.h"
#include "services/window_manager/window_manager_app.h"
#include "services/window_manager/window_manager_delegate.h"
#include "services/window_manager/window_manager_root.h"

// ApplicationDelegate implementation file for WindowManager users (e.g.
// core window manager tests) that do not want to provide their own
// ApplicationDelegate::Create().

using mojo::View;
using mojo::ViewManager;

namespace window_manager {

class DefaultWindowManagerController : public WindowManagerController {
 public:
  DefaultWindowManagerController(WindowManagerRoot* wm_root)
      : window_manager_root_(wm_root), root_(nullptr), window_offset_(10) {}

  ~DefaultWindowManagerController() override {}

 private:
  // Overridden from ViewManagerDelegate:
  void OnEmbed(View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    root_ = root;
    window_manager_root_->InitFocus(
        make_scoped_ptr(new window_manager::BasicFocusRules(root_)));
  }

  void OnViewManagerDisconnected(ViewManager* view_manager) override {}

  // Overridden from WindowManagerDelegate:
  void Embed(const mojo::String& url,
             mojo::InterfaceRequest<mojo::ServiceProvider> services,
             mojo::ServiceProviderPtr exposed_services) override {
    DCHECK(root_);
    View* view = root_->view_manager()->CreateView();
    root_->AddChild(view);

    mojo::Rect rect;
    rect.x = rect.y = window_offset_;
    rect.width = rect.height = 100;
    view->SetBounds(rect);
    window_offset_ += 10;

    view->SetVisible(true);
    view->Embed(url, services.Pass(), exposed_services.Pass());
  }

  WindowManagerRoot* window_manager_root_;

  View* root_;
  int window_offset_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(DefaultWindowManagerController);
};

class DefaultWindowManager : public mojo::ApplicationDelegate,
                             public WindowManagerControllerFactory {
 public:
  DefaultWindowManager() : window_manager_app_(new WindowManagerApp(this)) {}

  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* impl) override {
    tracing_.Initialize(impl);
    window_manager_app_->Initialize(impl);
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    window_manager_app_->ConfigureIncomingConnection(connection);
    return true;
  }

  scoped_ptr<WindowManagerController> CreateWindowManagerController(
      mojo::ApplicationConnection* connection,
      window_manager::WindowManagerRoot* wm_root) override {
    return scoped_ptr<WindowManagerController>(
        new DefaultWindowManagerController(wm_root));
  }

 private:
  scoped_ptr<WindowManagerApp> window_manager_app_;
  mojo::TracingImpl tracing_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(DefaultWindowManager);
};

}  // namespace window_manager

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new window_manager::DefaultWindowManager);
  return runner.Run(application_request);
}
