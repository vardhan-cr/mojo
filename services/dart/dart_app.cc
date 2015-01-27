// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/dart_app.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/isolate_data.h"
#include "mojo/public/cpp/bindings/interface_request.h"

using mojo::Application;

namespace dart {

DartApp::DartApp(mojo::InterfaceRequest<Application> application_request,
                 mojo::URLResponsePtr response)
    : application_request_(application_request.Pass()) {
  DCHECK(!response.is_null());
  std::string url(response->url);
  std::string source;
  CHECK(mojo::common::BlockingCopyToString(response->body.Pass(), &source));

  // TODO(zra): Where is the package root? For now, use DIR_EXE/gen.
  base::FilePath package_root;
  PathService::Get(base::DIR_EXE, &package_root);
  package_root = package_root.AppendASCII("gen");

  config_.application_data = reinterpret_cast<void*>(this);
  config_.script = source;
  config_.script_uri = url;
  config_.package_root = package_root.AsUTF8Unsafe();
  config_.entropy = nullptr;
  config_.arguments = nullptr;
  config_.arguments_count = 0;
  config_.compile_all = false;

  base::MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&DartApp::OnAppLoaded, base::Unretained(this)));
}

DartApp::~DartApp() {
}

void DartApp::OnAppLoaded() {
  char* error = nullptr;
  config_.handle = application_request_.PassMessagePipe().release().value();
  config_.error = &error;
  bool success = mojo::dart::DartController::RunDartScript(config_);
  if (!success) {
    LOG(ERROR) << error;
    free(error);
  }
  base::MessageLoop::current()->QuitWhenIdle();
}

}  // namespace dart
