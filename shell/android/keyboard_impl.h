// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_KEYBOARD_IMPL_H_
#define SHELL_ANDROID_KEYBOARD_IMPL_H_

#include <jni.h>

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"

namespace shell {

class KeyboardImpl {
 public:
  static void CreateKeyboardImpl(
      mojo::InterfaceRequest<keyboard::KeyboardService> request);
};

bool RegisterKeyboardJni(JNIEnv* env);

}  // namespace shell

#endif  // SHELL_ANDROID_KEYBOARD_IMPL_H_
