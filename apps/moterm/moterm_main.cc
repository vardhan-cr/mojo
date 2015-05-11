// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This is the "main" for the embeddable Moterm terminal view, which provides
// services to the thing embedding it. (This is not very useful as a "top-level"
// application.)

#include "apps/moterm/moterm_view.h"
#include "base/logging.h"
#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"

namespace {

class Moterm : public mojo::ApplicationDelegate,
               public mojo::ViewManagerDelegate {
 public:
  Moterm() : application_impl_() {}
  ~Moterm() override {}

 private:
  // |mojo::ApplicationDelegate|:
  void Initialize(mojo::ApplicationImpl* application_impl) override {
    DCHECK(!application_impl_);
    application_impl_ = application_impl;
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(application_impl_->shell(), this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // |mojo::ViewManagerDelegate|:
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    new MotermView(application_impl_->shell(), root, services.Pass());
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
    base::MessageLoop::current()->Quit();
  }

  mojo::ApplicationImpl* application_impl_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(Moterm);
};

}  // namespace

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new Moterm());
  return runner.Run(application_request);
}
