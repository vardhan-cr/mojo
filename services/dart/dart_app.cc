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
#include "mojo/dart/embedder/isolate_data.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/zlib/google/zip_reader.h"

using mojo::Application;

namespace dart {

DartApp::DartApp(mojo::InterfaceRequest<Application> application_request,
                 mojo::URLResponsePtr response,
                 bool strict)
    : application_request_(application_request.Pass()) {
  DCHECK(!response.is_null());
  std::string url(response->url);

  CHECK(unpacked_app_directory_.CreateUniqueTempDir());
  ExtractApplication(response.Pass());
  base::FilePath package_root = unpacked_app_directory_.path();

  base::FilePath entry_path = package_root.Append("main.dart");
  std::string source;
  if (!base::ReadFileToString(entry_path, &source)) {
    NOTREACHED();
    return;
  }

  config_.application_data = reinterpret_cast<void*>(this);
  config_.strict_compilation = strict;
  config_.script = source;
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

void DartApp::ExtractApplication(mojo::URLResponsePtr response) {
  zip::ZipReader reader;
  const std::string input_data = CopyToString(response->body.Pass());
  CHECK(reader.OpenFromString(input_data));
  base::FilePath temp_dir_path = unpacked_app_directory_.path();
  while (reader.HasMore()) {
    CHECK(reader.OpenCurrentEntryInZip());
    CHECK(reader.ExtractCurrentEntryIntoDirectory(temp_dir_path));
    CHECK(reader.AdvanceToNextEntry());
  }
}

std::string DartApp::CopyToString(mojo::ScopedDataPipeConsumerHandle body) {
  std::string body_str;
  bool result = mojo::common::BlockingCopyToString(body.Pass(), &body_str);
  DCHECK(result);
  return body_str;
}

}  // namespace dart
