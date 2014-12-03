// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
#define MOJO_DART_EMBEDDER_DART_CONTROLLER_H_

#include "dart/runtime/include/dart_api.h"

namespace mojo {
namespace dart {

class DartController {
 public:
  static bool runDartScript(void* dart_app,
                            const std::string& script,
                            const std::string& script_uri,
                            const std::string& package_root,
                            Dart_IsolateCreateCallback create,
                            Dart_IsolateShutdownCallback shutdown,
                            Dart_EntropySource entropy,
                            const char** extra_args,
                            int num_extra_args,
                            char** error);

 private:
  static void initVmIfNeeded(Dart_IsolateShutdownCallback shutdown,
                             Dart_EntropySource entropy,
                             const char** extra_args,
                             int num_extra_args);
  static bool vmIsInitialized;
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_DART_CONTROLLER_H_
