// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "examples/recipes/window_manager/window_manager.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "services/window_manager/basic_focus_rules.h"
#include "services/window_manager/window_manager_app.h"
#include "services/window_manager/window_manager_delegate.h"

namespace recipes {
namespace window_manager {

class Main : public mojo::ApplicationDelegate,
             public mojo::ViewManagerDelegate,
             public ::window_manager::WindowManagerDelegate {
 public:
  Main()
      : window_manager_app_(
            new ::window_manager::WindowManagerApp(this, this)) {}
  ~Main() override {}

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* impl) override {
    window_manager_app_->Initialize(impl);

    for (size_t i = 1; i < impl->args().size(); ++i) {
      mojo::InterfaceRequest<mojo::ServiceProvider> empty_request;
      window_manager_app_->Embed(impl->args()[i], empty_request.Pass());
    }
  }
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    window_manager_app_->ConfigureIncomingConnection(connection);
    return true;
  }

  // Overridden from mojo::ViewManagerDelegate:
  void OnEmbed(
      mojo::View* root,
      mojo::ServiceProviderImpl* exported_services,
      scoped_ptr<mojo::ServiceProvider> remote_service_provider) override {
    window_manager_.reset(new WindowManager(root));

    window_manager_app_->InitFocus(
        make_scoped_ptr(new ::window_manager::BasicFocusRules(root)));
  }
  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
    // TODO(sky): quit here?
  }

  // Overridden from ::window_manager::WindowManagerDelegate:
  void Embed(
      const mojo::String& url,
      mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) override {
    DCHECK(window_manager_.get());
    mojo::View* view = window_manager_->Create();
    view->Embed(url, service_provider.Pass());
  }

  scoped_ptr<::window_manager::WindowManagerApp> window_manager_app_;
  scoped_ptr<WindowManager> window_manager_;

  DISALLOW_COPY_AND_ASSIGN(Main);
};

}  // window_manager
}  // recipes

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new recipes::window_manager::Main);
  return runner.Run(shell_handle);
}
