// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_DART_SNAPSHOTTER_LOADER_H_
#define MOJO_DART_DART_SNAPSHOTTER_LOADER_H_

#include <string>

#include "base/files/file_path.h"
#include "dart/runtime/include/dart_api.h"

void InitLoader(const base::FilePath& package_root);
Dart_Handle HandleLibraryTag(Dart_LibraryTag tag,
                             Dart_Handle library,
                             Dart_Handle url);
void LoadScript(const std::string& url);

#endif  // MOJO_DART_DART_SNAPSHOTTER_LOADER_H_
