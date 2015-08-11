// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/at_exit.h"
#include "base/basictypes.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/process/memory.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/dart_snapshotter/loader.h"
#include "mojo/dart/dart_snapshotter/vm.h"
#include "tonic/dart_error.h"
#include "tonic/dart_isolate_scope.h"

const char kHelp[] = "help";
const char kPackageRoot[] = "package-root";
const char kSnapshot[] = "snapshot";

void Usage() {
  std::cerr << "Usage: dart_snapshotter"
            << " --" << kPackageRoot << " --" << kSnapshot
            << " <dart-app>" << std::endl;
}

void WriteSnapshot(base::FilePath path) {
  uint8_t* buffer;
  intptr_t size;
  CHECK(!tonic::LogIfError(Dart_CreateScriptSnapshot(&buffer, &size)));

  CHECK_EQ(base::WriteFile(path, reinterpret_cast<const char*>(buffer), size),
           size);
}

int main(int argc, const char* argv[]) {
  base::AtExitManager exit_manager;
  base::EnableTerminationOnHeapCorruption();
  base::CommandLine::Init(argc, argv);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(kHelp) || command_line.GetArgs().empty()) {
    Usage();
    return 0;
  }

  InitDartVM();

  CHECK(command_line.HasSwitch(kPackageRoot)) << "Need --package-root";
  InitLoader(command_line.GetSwitchValuePath(kPackageRoot));

  Dart_Isolate isolate = CreateDartIsolate();
  CHECK(isolate);

  tonic::DartIsolateScope scope(isolate);
  tonic::DartApiScope api_scope;

  auto args = command_line.GetArgs();
  CHECK(args.size() == 1);
  LoadScript(args[0]);

  CHECK(!tonic::LogIfError(Dart_FinalizeLoading(true)));

  CHECK(command_line.HasSwitch(kSnapshot)) << "Need --snapshot";
  WriteSnapshot(command_line.GetSwitchValuePath(kSnapshot));

  return 0;
}
