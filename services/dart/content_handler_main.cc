// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/icu/icu.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/dart/dart_app.h"

namespace dart {

class DartContentHandler : public mojo::ApplicationDelegate,
                           public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  DartContentHandler() : content_handler_factory_(this) {}
  ~DartContentHandler() {
    mojo::dart::DartController::Shutdown();
  }

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    mojo::icu::Initialize(app);
    // TODO(zra): Maybe have two different content handlers, one with checking
    // and one without.
    bool success = mojo::dart::DartController::Initialize(true);
    if (!success) {
      LOG(ERROR) << "Dart VM Initialization failed";
    }
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(mojo::ShellPtr shell,
                    mojo::URLResponsePtr response) override {
    return make_scoped_ptr(new DartApp(shell.Pass(), response.Pass()));
  }

  mojo::ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandler);
};

}  // namespace dart

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new dart::DartContentHandler);
  return runner.Run(shell_handle);
}
