// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/trace_event/trace_event.h"
#include "examples/ganesh_app/ganesh_view.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"

namespace examples {

class GaneshApp : public mojo::ApplicationDelegate,
                  public mojo::ViewManagerDelegate {
 public:
  GaneshApp() {}
  ~GaneshApp() override {}

  void Initialize(mojo::ApplicationImpl* app) override {
    tracing_.Initialize(app);
    TRACE_EVENT0("ganesh_app", __func__);
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(app->shell(), this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    TRACE_EVENT0("ganesh_app", __func__);
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    TRACE_EVENT0("ganesh_app", __func__);
    new GaneshView(shell_, root);
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
    base::MessageLoop::current()->Quit();
  }

 private:
  mojo::TracingImpl tracing_;
  mojo::Shell* shell_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(GaneshApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::GaneshApp);
  return runner.Run(application_request);
}
