// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/stringprintf.h"
#include "examples/bitmap_uploader/bitmap_uploader.h"
#include "examples/window_manager/window_manager.mojom.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "third_party/skia/include/core/SkColor.h"
#include "url/gurl.h"

namespace mojo {
namespace examples {

namespace {
const char kEmbeddedAppURL[] = "mojo:embedded_app";
}

class NestingApp;

// An app that embeds another app.
// TODO(davemoore): Is this the right name?
class NestingApp
    : public ApplicationDelegate,
      public ViewManagerDelegate,
      public ViewObserver {
 public:
  NestingApp() : nested_(nullptr), shell_(nullptr) {}
  ~NestingApp() override {}

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new ViewManagerClientFactory(app->shell(), this));
  }

  // Overridden from ApplicationImpl:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->ConnectToService(&window_manager_);
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // Overridden from ViewManagerDelegate:
  void OnEmbed(View* root,
               InterfaceRequest<ServiceProvider> services,
               ServiceProviderPtr exposed_services) override {
    root->AddObserver(this);
    bitmap_uploader_.reset(new BitmapUploader(root));
    bitmap_uploader_->Init(shell_);
    bitmap_uploader_->SetColor(SK_ColorCYAN);

    nested_ = root->view_manager()->CreateView();
    root->AddChild(nested_);
    Rect rect;
    rect.x = rect.y = 20;
    rect.width = rect.height = 50;
    nested_->SetBounds(rect);
    nested_->SetVisible(true);
    nested_->Embed(kEmbeddedAppURL);
  }
  void OnViewManagerDisconnected(ViewManager* view_manager) override {
   base::MessageLoop::current()->Quit();
  }

  // Overridden from ViewObserver:
  void OnViewDestroyed(View* view) override {
    // TODO(beng): reap views & child Views.
    nested_ = NULL;
  }
  void OnViewInputEvent(View* view, const EventPtr& event) override {
    if (event->action == EventType::POINTER_UP)
      window_manager_->CloseWindow(view->id());
  }

  scoped_ptr<ViewManagerClientFactory> view_manager_client_factory_;

  View* nested_;
  Shell* shell_;
  ::examples::IWindowManagerPtr window_manager_;
  scoped_ptr<BitmapUploader> bitmap_uploader_;

  DISALLOW_COPY_AND_ASSIGN(NestingApp);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::NestingApp);
  return runner.Run(application_request);
}
