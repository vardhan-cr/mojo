// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
#define MOJO_DART_EMBEDDER_DART_CONTROLLER_H_

#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/isolate_data.h"
#include "mojo/public/c/system/types.h"

namespace mojo {
namespace dart {

struct DartControllerConfig {
  void* application_data;
  std::string script;
  std::string script_uri;
  std::string package_root;
  IsolateCallbacks callbacks;
  Dart_EntropySource entropy;
  const char** arguments;
  int arguments_count;
  MojoHandle handle;
  // TODO(zra): search for the --compile_all flag in arguments where needed.
  bool compile_all;
  char** error;
};

class DartController {
 public:
  // Initializes the VM, starts up Dart's handle watcher, and runs the script
  // config.script to completion. This function returns when the script exits
  // and the handle watcher is shutdown. If you need to run multiple Dart
  // scripts in the same VM, use the calls below.
  static bool RunSingleDartScript(const DartControllerConfig& config);

  // Initializes the Dart VM, and starts up Dart's handle watcher.
  // If checked_mode is true, asserts and type checks are enabled.
  static bool Initialize(bool checked_mode);

  // Assumes Initialize has been called. Runs the main function using the
  // script, arguments, and package_root given by 'config'.
  static bool RunDartScript(const DartControllerConfig& config);

  // Waits for the handle watcher isolate to finish and shuts down the VM.
  static void Shutdown();

 private:
  static void InitVmIfNeeded(Dart_EntropySource entropy,
                             const char** arguments,
                             int arguments_count);
  static bool initialized_;
  static Dart_Isolate root_isolate_;
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
