// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_BUILTIN_H_
#define MOJO_DART_EMBEDDER_BUILTIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base/macros.h"
#include "dart/runtime/include/dart_api.h"

namespace mojo {
namespace dart {

#define REGISTER_FUNCTION(name, count)                                         \
  { "" #name, name, count },
#define DECLARE_FUNCTION(name, count)                                          \
  extern void name(Dart_NativeArguments args);

class Builtin {
 public:
  // Note: Changes to this enum should be accompanied with changes to
  // the builtin_libraries_ array in builtin.cc.
  enum BuiltinLibraryId {
    kBuiltinLibrary = 0,
    kMojoBindingsLibrary = 1,
    kMojoCoreLibrary = 2,
    kInvalidLibrary,
  };

  static uint8_t snapshot_magic_number[4];

  static void SetNativeResolver(BuiltinLibraryId id);

  // Check if built in library specified in 'id' is already loaded, if not
  // load it.
  static Dart_Handle LoadAndCheckLibrary(BuiltinLibraryId id);

  static int64_t GetIntegerValue(Dart_Handle value_obj) {
    int64_t value = 0;
    Dart_Handle result = Dart_IntegerToInt64(value_obj, &value);
    if (Dart_IsError(result))
      Dart_PropagateError(result);
    return value;
  }

  static Dart_Handle NewError(const char* format, ...);

 private:
  // Native method support.
  static Dart_NativeFunction NativeLookup(Dart_Handle name,
                                          int argument_count,
                                          bool* auto_setup_scope);

  static const uint8_t* NativeSymbol(Dart_NativeFunction nf);

  typedef struct {
    const char* url_;
    bool has_natives_;
    Dart_NativeEntrySymbol native_symbol_;
    Dart_NativeEntryResolver native_resolver_;
  } builtin_lib_props;
  static builtin_lib_props builtin_libraries_[];

  DISALLOW_IMPLICIT_CONSTRUCTORS(Builtin);
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_BUILTIN_H_
