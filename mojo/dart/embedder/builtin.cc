// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "base/files/file_util.h"
#include "base/i18n/icu_util.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/mojo_natives.h"

namespace mojo {
namespace dart {

Builtin::builtin_lib_props Builtin::builtin_libraries_[] = {
    /* { url_, has_natives_, native_symbol_, native_resolver_ } */
    {"dart:mojo_builtin", true, Builtin::NativeSymbol, Builtin::NativeLookup},
    {"dart:mojo_bindings", false, nullptr, nullptr},
    {"dart:mojo_core", true, MojoNativeSymbol, MojoNativeLookup},
};

uint8_t Builtin::snapshot_magic_number[] = {0xf5, 0xf5, 0xdc, 0xdc};

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

void Builtin::SetNativeResolver(BuiltinLibraryId id) {
  static_assert((sizeof(builtin_libraries_) / sizeof(builtin_lib_props)) ==
                kInvalidLibrary, "Unexpected number of builtin libraries");
  DCHECK_GE(id, kBuiltinLibrary);
  DCHECK_LT(id, kInvalidLibrary);
  if (builtin_libraries_[id].has_natives_) {
    Dart_Handle url = Dart_NewStringFromCString(builtin_libraries_[id].url_);
    Dart_Handle library = Dart_LookupLibrary(url);
    DCHECK(!Dart_IsError(library));
    // Setup the native resolver for built in library functions.
    DART_CHECK_VALID(
        Dart_SetNativeResolver(library,
                               builtin_libraries_[id].native_resolver_,
                               builtin_libraries_[id].native_symbol_));
  }
}

Dart_Handle Builtin::LoadAndCheckLibrary(BuiltinLibraryId id) {
  static_assert((sizeof(builtin_libraries_) / sizeof(builtin_lib_props)) ==
                kInvalidLibrary, "Unexpected number of builtin libraries");
  DCHECK_GE(id, kBuiltinLibrary);
  DCHECK_LT(id, kInvalidLibrary);
  Dart_Handle url = Dart_NewStringFromCString(builtin_libraries_[id].url_);
  Dart_Handle library = Dart_LookupLibrary(url);
  DART_CHECK_VALID(library);
  return library;
}

}  // namespace dart
}  // namespace mojo
