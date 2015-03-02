// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace mojo {
namespace examples {

class PrintBodyApplication : public Application {
 public:
  PrintBodyApplication(InterfaceRequest<Application> request,
                       ScopedDataPipeConsumerHandle body)
      : binding_(this, request.Pass()), body_(body.Pass()) {}

  virtual void Initialize(ShellPtr shell,
                          Array<String> args,
                          const mojo::String& url) override {
    shell_ = shell.Pass();
  }
  virtual void RequestQuit() override {}

  virtual void AcceptConnection(const String& requestor_url,
                                InterfaceRequest<ServiceProvider> services,
                                ServiceProviderPtr exported_services,
                                const String& url) override {
    printf(
        "ContentHandler::OnConnect - url:%s - requestor_url:%s - body "
        "follows\n\n",
        url.To<std::string>().c_str(), requestor_url.To<std::string>().c_str());
    PrintResponse(body_.Pass());
    delete this;
  }

 private:
  void PrintResponse(ScopedDataPipeConsumerHandle body) const {
    for (;;) {
      char buf[512];
      uint32_t num_bytes = sizeof(buf);
      MojoResult result = ReadDataRaw(body.get(), buf, &num_bytes,
                                      MOJO_READ_DATA_FLAG_NONE);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        Wait(body.get(),
             MOJO_HANDLE_SIGNAL_READABLE,
             MOJO_DEADLINE_INDEFINITE,
             nullptr);
      } else if (result == MOJO_RESULT_OK) {
        if (fwrite(buf, num_bytes, 1, stdout) != 1) {
          printf("\nUnexpected error writing to file\n");
          break;
        }
      } else {
        break;
      }

      printf("\n>>> EOF <<<\n");
    }
  }

  StrongBinding<Application> binding_;
  ShellPtr shell_;
  ScopedDataPipeConsumerHandle body_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(PrintBodyApplication);
};

class ContentHandlerImpl : public ContentHandler {
 public:
  explicit ContentHandlerImpl(InterfaceRequest<ContentHandler> request)
      : binding_(this, request.Pass()) {}
  ~ContentHandlerImpl() override {}

 private:
  void StartApplication(InterfaceRequest<Application> application,
                        URLResponsePtr response) override {
    // The application will delete itself after being connected to.
    new PrintBodyApplication(application.Pass(), response->body.Pass());
  }

  StrongBinding<ContentHandler> binding_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(ContentHandlerImpl);
};

class ContentHandlerApp : public ApplicationDelegate,
                          public InterfaceFactory<ContentHandler> {
 public:
  ContentHandlerApp() {}
  ~ContentHandlerApp() override {}

  void Initialize(ApplicationImpl* app) override {}

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

  void Create(ApplicationConnection* app,
              InterfaceRequest<ContentHandler> request) override {
    new ContentHandlerImpl(request.Pass());
  }

 private:

  MOJO_DISALLOW_COPY_AND_ASSIGN(ContentHandlerApp);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunner runner(new mojo::examples::ContentHandlerApp);
  return runner.Run(shell_handle);
}
