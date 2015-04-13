// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string.h>

#include "base/logging.h"
#include "base/macros.h"
#include "dart/runtime/include/dart_api.h"
#include "mojo/dart/embedder/builtin.h"
#include "mojo/dart/embedder/common.h"
#include "mojo/dart/embedder/io/internet_address.h"

namespace mojo {
namespace dart {

#define MOJO_IO_NATIVE_LIST(V)                                                 \
  V(InternetAddress_Parse, 1)                                                  \

MOJO_IO_NATIVE_LIST(DECLARE_FUNCTION);

static struct NativeEntries {
  const char* name;
  Dart_NativeFunction function;
  int argument_count;
} MojoEntries[] = {MOJO_IO_NATIVE_LIST(REGISTER_FUNCTION)};

Dart_NativeFunction MojoIoNativeLookup(Dart_Handle name,
                                       int argument_count,
                                       bool* auto_setup_scope) {
  const char* function_name = nullptr;
  Dart_Handle result = Dart_StringToCString(name, &function_name);
  DART_CHECK_VALID(result);
  DCHECK(function_name != nullptr);
  DCHECK(auto_setup_scope != nullptr);
  *auto_setup_scope = true;
  size_t num_entries = arraysize(MojoEntries);
  for (size_t i = 0; i < num_entries; ++i) {
    const struct NativeEntries& entry = MojoEntries[i];
    if (!strcmp(function_name, entry.name) &&
        (entry.argument_count == argument_count)) {
      return entry.function;
    }
  }
  return nullptr;
}

const uint8_t* MojoIoNativeSymbol(Dart_NativeFunction nf) {
  size_t num_entries = arraysize(MojoEntries);
  for (size_t i = 0; i < num_entries; ++i) {
    const struct NativeEntries& entry = MojoEntries[i];
    if (entry.function == nf) {
      return reinterpret_cast<const uint8_t*>(entry.name);
    }
  }
  return nullptr;
}

void InternetAddress_Parse(Dart_NativeArguments arguments) {
  const char* address = DartEmbedder::GetStringArgument(arguments, 0);
  CHECK(address != NULL);
  RawAddr raw;
  int type = strchr(address, ':') == NULL ? InternetAddress::TYPE_IPV4
                                          : InternetAddress::TYPE_IPV6;
  intptr_t length = (type == InternetAddress::TYPE_IPV4) ?
      IPV4_RAW_ADDR_LENGTH : IPV6_RAW_ADDR_LENGTH;
  if (InternetAddress::Parse(type, address, &raw)) {
    Dart_SetReturnValue(arguments,
                        DartEmbedder::MakeUint8TypedData(&raw.bytes[0],
                                                         length));
  } else {
    DartEmbedder::SetNullReturn(arguments);
  }
}

}  // namespace dart
}  // namespace mojo
