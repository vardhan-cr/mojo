// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/message_loop/message_loop.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "services/keyboard_native/keyboard_service_impl.h"

namespace keyboard {

class KeyboardServiceFactory : public mojo::InterfaceFactory<KeyboardService> {
 public:
  explicit KeyboardServiceFactory(
      mojo::InterfaceRequest<mojo::ServiceProvider> service_provider_request) {
    if (service_provider_request.is_pending()) {
      service_provider_impl_.Bind(service_provider_request.Pass());
      service_provider_impl_.AddService<KeyboardService>(this);
    }
  }
  ~KeyboardServiceFactory() override {}

  void OnViewCreated(mojo::View* view, mojo::Shell* shell) {
    view_observer_delegate_.OnViewCreated(view, shell);
  }

  // mojo::InterfaceFactory<KeyboardService> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<KeyboardService> request) override {
    KeyboardServiceImpl* keyboard_service_impl =
        new KeyboardServiceImpl(request.Pass());
    view_observer_delegate_.SetKeyboardServiceImpl(keyboard_service_impl);
  }

 private:
  mojo::ServiceProviderImpl service_provider_impl_;
  ViewObserverDelegate view_observer_delegate_;
};

class KeyboardServiceDelegate : public mojo::ApplicationDelegate,
                                public mojo::ViewManagerDelegate {
 public:
  KeyboardServiceDelegate() : shell_(nullptr) {}
  ~KeyboardServiceDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(shell_, this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // mojo::ViewManagerDelegate implementation.
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    KeyboardServiceFactory* keyboard_service_factory =
        new KeyboardServiceFactory(services.Pass());
    keyboard_service_factory->OnViewCreated(root, shell_);
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
    base::MessageLoop::current()->Quit();
  }

 private:
  mojo::Shell* shell_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceDelegate);
};

}  // namespace keyboard

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new keyboard::KeyboardServiceDelegate());
  return runner.Run(application_request);
}
