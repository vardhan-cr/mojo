// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include "mojo/dart/embedder/common.h"

namespace mojo {
namespace dart {

int64_t DartEmbedder::GetIntegerValue(Dart_Handle value_obj) {
  int64_t value = 0;
  Dart_Handle result = Dart_IntegerToInt64(value_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return value;
}

int64_t DartEmbedder::GetInt64ValueCheckRange(
    Dart_Handle value_obj, int64_t lower, int64_t upper) {
  int64_t value = DartEmbedder::GetIntegerValue(value_obj);
  if (value < lower || upper < value) {
    Dart_PropagateError(Dart_NewApiError("Value outside expected range"));
  }
  return value;
}

intptr_t DartEmbedder::GetIntptrValue(Dart_Handle value_obj) {
  int64_t value = 0;
  Dart_Handle result = Dart_IntegerToInt64(value_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  if (value < INTPTR_MIN || INTPTR_MAX < value) {
    Dart_PropagateError(Dart_NewApiError("Value outside intptr_t range"));
  }
  return static_cast<intptr_t>(value);
}

const char* DartEmbedder::GetStringValue(Dart_Handle str_obj) {
  const char* cstring = nullptr;
  Dart_Handle result = Dart_StringToCString(str_obj, &cstring);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return cstring;
}

bool DartEmbedder::GetBooleanValue(Dart_Handle bool_obj) {
  bool value = false;
  Dart_Handle result = Dart_BooleanValue(bool_obj, &value);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return value;
}

void DartEmbedder::SetIntegerField(Dart_Handle handle,
                                   const char* name,
                                   int64_t val) {
  Dart_Handle result = Dart_SetField(handle,
                                     NewCString(name),
                                     Dart_NewInteger(val));
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}

void DartEmbedder::SetStringField(Dart_Handle handle,
                                  const char* name,
                                  const char* val) {
  Dart_Handle result = Dart_SetField(handle, NewCString(name), NewCString(val));
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
}

Dart_Handle DartEmbedder::NewCString(const char* str) {
  CHECK(str != nullptr);
  Dart_Handle result = Dart_NewStringFromCString(str);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return result;
}

void DartEmbedder::SetNullReturn(Dart_NativeArguments arguments) {
  Dart_SetReturnValue(arguments, Dart_Null());
}

const char* DartEmbedder::GetStringArgument(Dart_NativeArguments arguments,
                                            intptr_t index) {
  Dart_Handle str_arg = Dart_GetNativeArgument(arguments, index);
  const char* cstring = NULL;
  Dart_Handle result = Dart_StringToCString(str_arg, &cstring);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  return cstring;
}

Dart_Handle DartEmbedder::MakeUint8TypedData(uint8_t* bytes, intptr_t length) {
  Dart_Handle result = Dart_NewTypedData(Dart_TypedData_kUint8, length);
  if (Dart_IsError(result)) {
    Dart_PropagateError(result);
  }
  Dart_Handle err = Dart_ListSetAsBytes(result, 0, bytes, length);
  if (Dart_IsError(err)) {
    Dart_PropagateError(err);
  }
  return result;
}

}  // namespace dart
}  // namespace mojo
