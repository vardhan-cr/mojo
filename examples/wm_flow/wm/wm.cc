// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/wm_flow/wm/frame_controller.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "services/window_manager/basic_focus_rules.h"
#include "services/window_manager/window_manager_app.h"
#include "services/window_manager/window_manager_delegate.h"
#include "url/gurl.h"

namespace examples {

class SimpleWM : public mojo::ApplicationDelegate,
                 public mojo::ViewManagerDelegate,
                 public window_manager::WindowManagerDelegate,
                 public mojo::ViewObserver {
 public:
  SimpleWM()
      : window_manager_app_(new window_manager::WindowManagerApp(this, this)),
        root_(NULL),
        window_container_(NULL),
        next_window_origin_(10, 10) {}
  virtual ~SimpleWM() {}

 private:
  // Overridden from mojo::ApplicationDelegate:
  virtual void Initialize(mojo::ApplicationImpl* impl) override {
    // FIXME: Mojo applications don't know their URLs yet:
    // https://docs.google.com/a/chromium.org/document/d/1AQ2y6ekzvbdaMF5WrUQmneyXJnke-MnYYL4Gz1AKDos
    url_ = GURL(impl->args()[1]);
    window_manager_app_->Initialize(impl);
  }
  virtual bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    window_manager_app_->ConfigureIncomingConnection(connection);
    return true;
  }

  // Overridden from mojo::ViewManagerDelegate:
  virtual void OnEmbed(
      mojo::View* root,
      mojo::ServiceProviderImpl* exported_services,
      scoped_ptr<mojo::ServiceProvider> remote_service_provider) override {
    root_ = root;

    window_container_ = root->view_manager()->CreateView();
    window_container_->SetBounds(root_->bounds());
    root_->AddChild(window_container_);
    window_container_->SetVisible(true);

    window_manager_app_->InitFocus(make_scoped_ptr(
        new window_manager::BasicFocusRules(window_container_)));
  }
  virtual void OnViewManagerDisconnected(
      mojo::ViewManager* view_manager) override {
    root_ = NULL;
  }

  // Overridden from mojo::WindowManagerDelegate:
  virtual void Embed(
      const mojo::String& url,
      mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) override {
    DCHECK(root_);
    mojo::View* app_view = NULL;
    CreateTopLevelWindow(&app_view);
    app_view->Embed(url, service_provider.Pass());
  }

  // Overridden from mojo::ViewObserver:
  virtual void OnViewInputEvent(mojo::View* view,
                                const mojo::EventPtr& event) override {
    if (event->action == mojo::EVENT_TYPE_MOUSE_RELEASED &&
        event->flags & mojo::EVENT_FLAGS_RIGHT_MOUSE_BUTTON &&
        view->parent() == window_container_) {
      CloseWindow(view);
    }
  }
  virtual void OnViewDestroyed(mojo::View* view) override {
    view->RemoveObserver(this);
  }

  void CloseWindow(mojo::View* view) {
    mojo::View* first_child = view->children().front();
    first_child->Destroy();
    view->Destroy();
    next_window_origin_.Offset(-50, -50);
  }

  mojo::View* CreateTopLevelWindow(mojo::View** app_view) {
    mojo::View* frame_view = root_->view_manager()->CreateView();
    // Add the View to it's parent before showing so that animations can happen.
    window_container_->AddChild(frame_view);
    mojo::Rect rect;
    rect.x = next_window_origin_.x();
    rect.y = next_window_origin_.y();
    rect.width = rect.height = 400;
    frame_view->SetBounds(rect);
    next_window_origin_.Offset(50, 50);

    GURL frame_url = url_.Resolve("/examples/wm_flow/wm/window_frame.sky");
    new FrameController(frame_url, frame_view, app_view,
                        window_manager_app_.get());
    return frame_view;
  }

  scoped_ptr<window_manager::WindowManagerApp> window_manager_app_;

  GURL url_;
  mojo::View* root_;
  mojo::View* window_container_;

  gfx::Point next_window_origin_;

  DISALLOW_COPY_AND_ASSIGN(SimpleWM);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new examples::SimpleWM);
  return runner.Run(shell_handle);
}
