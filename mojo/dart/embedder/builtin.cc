// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/builtin.h"

namespace mojo {
namespace dart {

Builtin::builtin_lib_props Builtin::builtin_libraries_[] = {
    /* { url_, source_, has_natives_ } */
    {"dart:_builtin", _builtin_source_paths_, true},
};

uint8_t Builtin::snapshot_magic_number[] = {0xf5, 0xf5, 0xdc, 0xdc};

static Dart_Handle ReadStringFromFile(const char* str) {
  base::FilePath path(base::FilePath::FromUTF8Unsafe(str));
  std::string source;
  if (ReadFileToString(path, &source)) {
    Dart_Handle str = Dart_NewStringFromUTF8(
        reinterpret_cast<const uint8_t*>(source.c_str()), source.length());
    return str;
  } else {
    return Dart_Null();
  }
}


Dart_Handle Builtin::NewError(const char* format, ...) {
  va_list args;
  va_start(args, format);
  intptr_t len = vsnprintf(NULL, 0, format, args);
  va_end(args);

  char* buffer = reinterpret_cast<char*>(Dart_ScopeAllocate(len + 1));
  va_list args2;
  va_start(args2, format);
  vsnprintf(buffer, (len + 1), format, args2);
  va_end(args2);

  return Dart_NewApiError(buffer);
}


Dart_Handle Builtin::Source(BuiltinLibraryId id) {
  CHECK((sizeof(builtin_libraries_) / sizeof(builtin_lib_props)) ==
        kInvalidLibrary);
  CHECK(id >= kBuiltinLibrary && id < kInvalidLibrary);

  // Try to read the source using the path specified for the uri.
  const char* uri = builtin_libraries_[id].url_;
  const char** source_paths = builtin_libraries_[id].source_paths_;
  return GetSource(source_paths, uri);
}


Dart_Handle Builtin::PartSource(BuiltinLibraryId id, const char* part_uri) {
  CHECK((sizeof(builtin_libraries_) / sizeof(builtin_lib_props)) ==
        kInvalidLibrary);
  CHECK(id >= kBuiltinLibrary && id < kInvalidLibrary);

  // Try to read the source using the path specified for the uri.
  const char** source_paths = builtin_libraries_[id].source_paths_;
  return GetSource(source_paths, part_uri);
}


Dart_Handle Builtin::GetSource(const char** source_paths, const char* uri) {
  if (source_paths == NULL) {
    return Dart_Null();  // No path mapping information exists for library.
  }
  const char* source_path = NULL;
  for (intptr_t i = 0; source_paths[i] != NULL; i += 2) {
    if (!strcmp(uri, source_paths[i])) {
      source_path = source_paths[i + 1];
      break;
    }
  }
  if (source_path == NULL) {
    return Dart_Null();  // Uri does not exist in path mapping information.
  }
  return ReadStringFromFile(source_path);
}


Dart_Handle Builtin::LoadLibrary(Dart_Handle url, BuiltinLibraryId id) {
  Dart_Handle library = Dart_LoadLibrary(url, Source(id), 0, 0);
  if (!Dart_IsError(library) && (builtin_libraries_[id].has_natives_)) {
    // Setup the native resolver for built in library functions.
    DART_CHECK_VALID(
        Dart_SetNativeResolver(library, NativeLookup, NativeSymbol));
  }
  return library;
}


Dart_Handle Builtin::LoadAndCheckLibrary(BuiltinLibraryId id) {
  CHECK((sizeof(builtin_libraries_) / sizeof(builtin_lib_props)) ==
        kInvalidLibrary);
  CHECK(id >= kBuiltinLibrary && id < kInvalidLibrary);
  Dart_Handle url = Dart_NewStringFromCString(builtin_libraries_[id].url_);
  Dart_Handle library = Dart_LookupLibrary(url);
  if (Dart_IsError(library)) {
    library = LoadLibrary(url, id);
  }
  DART_CHECK_VALID(library);
  return library;
}

}  // namespace dart
}  // namespace mojo
