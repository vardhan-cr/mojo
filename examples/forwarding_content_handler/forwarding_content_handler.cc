// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace mojo {
namespace examples {

class ForwardingApplicationImpl : public Application {
 public:
  ForwardingApplicationImpl(InterfaceRequest<Application> request,
                            std::string target_url)
      : binding_(this, request.Pass()),
        target_url_(target_url) {
  }

 private:
  // Application:
  void Initialize(ShellPtr shell,
                  Array<String> args,
                  const mojo::String& url) override {
    shell_ = shell.Pass();
  }
  void AcceptConnection(const String& requestor_url,
                        InterfaceRequest<ServiceProvider> services,
                        ServiceProviderPtr exposed_services,
                        const String& requested_url) override {
    shell_->ConnectToApplication(target_url_, services.Pass(),
                                  exposed_services.Pass());
  }
  void RequestQuit() override {
    RunLoop::current()->Quit();
  }

  Binding<Application> binding_;
  std::string target_url_;
  ShellPtr shell_;
};

class ForwardingContentHandler : public ApplicationDelegate,
                                 public ContentHandlerFactory::ManagedDelegate {
 public:
  ForwardingContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(InterfaceRequest<Application> application_request,
                    URLResponsePtr response) override {
    CHECK(!response.is_null());
    const std::string requestor_url(response->url);
    std::string target_url;
    if(!common::BlockingCopyToString(response->body.Pass(), &target_url)) {
      LOG(WARNING) << "unable to read target URL from " << requestor_url;
      return nullptr;
    }
    return make_handled_factory_holder(
        new ForwardingApplicationImpl(application_request.Pass(), target_url));
  }

  ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(ForwardingContentHandler);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new mojo::examples::ForwardingContentHandler());
  return runner.Run(application_request);
}
