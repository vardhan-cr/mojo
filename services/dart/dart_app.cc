// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/dart_app.h"

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "crypto/random.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/isolate_data.h"
#include "mojo/public/cpp/bindings/interface_request.h"

namespace dart {

static bool generateEntropy(uint8_t* buffer, intptr_t length) {
  crypto::RandBytes(reinterpret_cast<void*>(buffer), length);
  return true;
}

DartApp::DartApp(mojo::ShellPtr shell, mojo::URLResponsePtr response)
    : shell_(shell.Pass()) {
  DCHECK(!response.is_null());
  std::string url(response->url);
  std::string source;
  CHECK(mojo::common::BlockingCopyToString(response->body.Pass(), &source));

  // TODO(zra): Where is the package root? For now, use DIR_EXE/gen.
  base::FilePath package_root;
  PathService::Get(base::DIR_EXE, &package_root);
  package_root = package_root.AppendASCII("gen");

  // TODO(zra): Instead of hard-coding these testing arguments here, parse them
  // out of the script, looking for vmoptions as we do in Dart VM tests.
  const int kNumArgs = 3;
  const char* args[kNumArgs];
  args[0] = "--enable_asserts";
  args[1] = "--enable_type_checks";
  args[2] = "--error_on_bad_type";

  config_.application_data = reinterpret_cast<void*>(this);
  config_.script = source;
  config_.script_uri = url;
  config_.package_root = package_root.AsUTF8Unsafe();
  config_.entropy = generateEntropy;
  config_.arguments = args;
  config_.arguments_count = kNumArgs;
  config_.compile_all = false;

  base::MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&DartApp::OnAppLoaded, base::Unretained(this)));
}

DartApp::~DartApp() {
}

void DartApp::OnAppLoaded() {
  char* error = nullptr;
  config_.handle = shell_.PassMessagePipe().release().value();
  config_.error = &error;
  bool success = mojo::dart::DartController::RunDartScript(config_);
  if (!success) {
    LOG(ERROR) << error;
    free(error);
  }
  base::MessageLoop::current()->QuitWhenIdle();
}

}  // namespace dart
