// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/dart_app.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/dart_state.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/zlib/google/zip_reader.h"

using mojo::Application;

namespace dart {

DartApp::DartApp(mojo::InterfaceRequest<Application> application_request,
                 const base::FilePath& application_dir,
                 bool strict)
    : application_request_(application_request.Pass()),
      application_dir_(application_dir) {
  base::FilePath package_root = application_dir_.AppendASCII("packages");
  base::FilePath entry_path = application_dir_.Append("main.dart");

  config_.application_data = reinterpret_cast<void*>(this);
  config_.strict_compilation = strict;
  config_.script_uri = entry_path.AsUTF8Unsafe();
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
