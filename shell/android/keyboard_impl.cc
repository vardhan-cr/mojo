// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/keyboard_impl.h"

#include "base/android/jni_android.h"
#include "base/logging.h"
#include "jni/Keyboard_jni.h"

namespace shell {

KeyboardImpl::KeyboardImpl(mojo::InterfaceRequest<Keyboard> request)
    : binding_(this, request.Pass()) {
}

KeyboardImpl::~KeyboardImpl() {
}

void KeyboardImpl::Show() {
  Java_Keyboard_showSoftKeyboard(base::android::AttachCurrentThread(),
                                 base::android::GetApplicationContext());
}

void KeyboardImpl::Hide() {
  Java_Keyboard_hideSoftKeyboard(base::android::AttachCurrentThread(),
                                 base::android::GetApplicationContext());
}

bool RegisterKeyboardJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace shell
