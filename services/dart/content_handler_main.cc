// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/icu/icu.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/dart/dart_app.h"
#include "url/gurl.h"

namespace dart {

class DartContentHandler : public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  DartContentHandler(bool strict) : strict_(strict) {}

 private:
  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    return make_scoped_ptr(
        new DartApp(application_request.Pass(), response.Pass(), strict_));
  }

  bool strict_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandler);
};

class DartContentHandlerApp : public mojo::ApplicationDelegate {
 public:
  DartContentHandlerApp()
      : content_handler_(false),
        strict_content_handler_(true),
        content_handler_factory_(&content_handler_),
        strict_content_handler_factory_(&strict_content_handler_) {}
  ~DartContentHandlerApp() { mojo::dart::DartController::Shutdown(); }

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    mojo::icu::Initialize(app);
    bool success = mojo::dart::DartController::Initialize(false);
    if (!success) {
      LOG(ERROR) << "Dart VM Initialization failed";
    }
  }

  bool HasStrictQueryParam(const std::string& requestedUrl) {
    bool strict_compilation = false;
    GURL url(requestedUrl);
    if (url.has_query()) {
      std::vector<std::string> query_parameters;
      Tokenize(url.query(), "&", &query_parameters);
      strict_compilation = std::find(query_parameters.begin(),
                                     query_parameters.end(),
                                     "strict=true")
                           != query_parameters.end();
    }
    return strict_compilation;
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    bool strict = HasStrictQueryParam(connection->GetConnectionURL());
    if (strict) {
      connection->AddService(&strict_content_handler_factory_);
    } else {
      connection->AddService(&content_handler_factory_);
    }
    return true;
  }

  DartContentHandler content_handler_;
  DartContentHandler strict_content_handler_;
  mojo::ContentHandlerFactory content_handler_factory_;
  mojo::ContentHandlerFactory strict_content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandlerApp);
};

}  // namespace dart

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new dart::DartContentHandlerApp);
  return runner.Run(shell_handle);
}
