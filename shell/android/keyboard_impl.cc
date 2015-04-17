// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/keyboard_impl.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "jni/Keyboard_jni.h"

namespace shell {

// static
void KeyboardImpl::CreateKeyboardImpl(
    mojo::InterfaceRequest<keyboard::KeyboardService> request) {
  MojoHandle handle_val = request.PassMessagePipe().release().value();
  Java_Keyboard_createKeyboardImpl(base::android::AttachCurrentThread(),
                                   base::android::GetApplicationContext(),
                                   handle_val);
}

bool RegisterKeyboardJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
