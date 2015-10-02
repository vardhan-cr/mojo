// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "examples/browser/browser_host.mojom.h"
#include "examples/window_manager/window_manager.mojom.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/navigation/public/interfaces/navigation.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "url/gurl.h"

namespace mojo {
namespace examples {

// This is the basics of creating a views widget with a textfield.
// TODO: cleanup!
class Browser : public ApplicationDelegate,
                public ViewManagerDelegate,
                public ViewObserver,
                public examples::BrowserHost,
                public mojo::InterfaceFactory<examples::BrowserHost> {
 public:
  Browser() : shell_(nullptr), root_(NULL), binding_(this) {
    browser_host_services_impl_.AddService(this);
  }

  ~Browser() override {
    if (root_)
      root_->RemoveObserver(this);
  }

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new ViewManagerClientFactory(shell_, this));
    app->ConnectToService("mojo:window_manager", &window_manager_);

    // FIXME: Mojo applications don't know their URLs yet:
    // https://docs.google.com/a/chromium.org/document/d/1AQ2y6ekzvbdaMF5WrUQmneyXJnke-MnYYL4Gz1AKDos
    url_ = GURL(app->args()[1]);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // ViewManagerDelegate:
  void OnEmbed(View* root,
               InterfaceRequest<ServiceProvider> services,
               ServiceProviderPtr exposed_services) override {
    // TODO: deal with OnEmbed() being invoked multiple times.
    ConnectToService(exposed_services.get(), &navigator_host_);
    root_ = root;
    root_->AddObserver(this);
    root_->SetFocus();

    // Create a child view for our sky document.
    View* view = root->view_manager()->CreateView();
    root->AddChild(view);
    Rect bounds;
    bounds.x = 0;
    bounds.y = 0;
    bounds.width = root->bounds().width;
    bounds.height = root->bounds().height;
    view->SetBounds(bounds);
    view->SetVisible(true);
    root->SetVisible(true);

    ServiceProviderPtr browser_host_services;
    browser_host_services_impl_.Bind(GetProxy(&browser_host_services));

    GURL frame_url = url_.Resolve("/examples/browser/browser.sky");
    view->Embed(frame_url.spec(), nullptr, browser_host_services.Pass());
  }

  void OnViewManagerDisconnected(ViewManager* view_manager) override {
    base::MessageLoop::current()->Quit();
  }

  // ViewObserver:
  void OnViewDestroyed(View* view) override {
    DCHECK_EQ(root_, view);
    view->RemoveObserver(this);
    root_ = NULL;
  }

  // examples::BrowserHost:
  void NavigateTo(const mojo::String& url) override {
    URLRequestPtr request(URLRequest::New());
    request->url = url;
    navigator_host_->RequestNavigate(Target::NEW_NODE, request.Pass());
  }

  // mojo::InterfaceFactory<examples::BrowserHost> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<examples::BrowserHost> request) override {
    binding_.Bind(request.Pass());
  }

  Shell* shell_;

  scoped_ptr<ViewManagerClientFactory> view_manager_client_factory_;
  View* root_;
  NavigatorHostPtr navigator_host_;
  ::examples::IWindowManagerPtr window_manager_;
  ServiceProviderImpl browser_host_services_impl_;

  GURL url_;

  mojo::Binding<examples::BrowserHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::Browser);
  return runner.Run(application_request);
}
