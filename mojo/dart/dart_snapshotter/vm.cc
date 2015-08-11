// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/dart/dart_snapshotter/vm.h"

#include "base/logging.h"
#include "mojo/dart/dart_snapshotter/loader.h"
#include "tonic/dart_error.h"
#include "tonic/dart_state.h"

namespace mojo {
namespace dart {
extern const uint8_t* vm_isolate_snapshot_buffer;
extern const uint8_t* isolate_snapshot_buffer;
}
}

static const char* kDartArgs[] = {
    "--enable_mirrors=false",
};

void InitDartVM() {
  CHECK(Dart_SetVMFlags(arraysize(kDartArgs), kDartArgs));
  CHECK(Dart_Initialize(mojo::dart::vm_isolate_snapshot_buffer, nullptr,
                        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                        nullptr, nullptr));
}

Dart_Isolate CreateDartIsolate() {
  CHECK(mojo::dart::isolate_snapshot_buffer);
  char* error = nullptr;
  Dart_Isolate isolate = Dart_CreateIsolate("dart:snapshot", "main",
                                            mojo::dart::isolate_snapshot_buffer,
                                            nullptr, nullptr, &error);

  CHECK(isolate) << error;
  CHECK(!tonic::LogIfError(Dart_SetLibraryTagHandler(HandleLibraryTag)));

  Dart_ExitIsolate();
  return isolate;
}
