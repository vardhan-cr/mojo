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

class DartContentHandler : public mojo::ApplicationDelegate,
                           public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  DartContentHandler() :
      first_connection_configured_(false),
      content_handler_factory_(this) {}
  ~DartContentHandler() {
    mojo::dart::DartController::Shutdown();
  }

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    mojo::icu::Initialize(app);
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
    if (!first_connection_configured_) {
      bool strict = HasStrictQueryParam(connection->GetConnectionURL());
      bool success = mojo::dart::DartController::Initialize(strict);
      if (!success) {
        LOG(ERROR) << "Dart VM Initialization failed";
        return false;
      }
      first_connection_configured_ = true;
    }
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    return make_scoped_ptr(
        new DartApp(application_request.Pass(), response.Pass()));
  }

  bool first_connection_configured_;
  mojo::ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandler);
};

}  // namespace dart

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new dart::DartContentHandler);
  return runner.Run(shell_handle);
}
