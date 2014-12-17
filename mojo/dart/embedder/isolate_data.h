// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_ISOLATE_DATA_H_
#define MOJO_DART_EMBEDDER_ISOLATE_DATA_H_

#include <stdlib.h>
#include <string.h>

#include "base/callback.h"
#include "base/macros.h"
#include "dart/runtime/include/dart_api.h"

namespace mojo {
namespace dart {

struct IsolateCallbacks {
  base::Callback<Dart_Isolate(const char*,const char*,const char*,void*,char**)>
      create;
  base::Callback<void(void*)> shutdown;
  base::Callback<void(Dart_Handle)> exception;
};

class IsolateData {
 public:
  IsolateData(void* app,
              IsolateCallbacks callbacks,
              const std::string& script,
              const std::string& script_uri,
              const std::string& package_root)
      : app(app),
        callbacks(callbacks),
        script(script),
        script_uri(script_uri),
        package_root(package_root) {}

  void* app;
  IsolateCallbacks callbacks;
  std::string script;
  std::string script_uri;
  std::string package_root;

  DISALLOW_COPY_AND_ASSIGN(IsolateData);
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_ISOLATE_DATA_H_
