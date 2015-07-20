// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DART_DART_APP_H_
#define SERVICES_DART_DART_APP_H_

#include "base/files/scoped_temp_dir.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"

namespace dart {

class DartApp;
class ApplicationDelegateImpl;

// Each Dart app started by content handler runs on its own thread and
// in its own Dart isolate. This class represents one running Dart app.

class DartApp : public mojo::ContentHandlerFactory::HandledApplicationHolder {
 public:
  DartApp(mojo::InterfaceRequest<mojo::Application> application_request,
          const base::FilePath& application_dir,
          bool strict);
  virtual ~DartApp();

 private:
  void OnAppLoaded();

  mojo::InterfaceRequest<mojo::Application> application_request_;
  mojo::dart::DartControllerConfig config_;
  base::FilePath application_dir_;

  DISALLOW_COPY_AND_ASSIGN(DartApp);
};

}  // namespace dart

#endif  // SERVICES_DART_DART_APP_H_
