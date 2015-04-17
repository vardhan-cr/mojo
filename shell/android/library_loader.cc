// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/base_jni_onload.h"
#include "base/android/base_jni_registrar.h"
#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "base/bind.h"
#include "mojo/android/system/core_impl.h"
#include "services/native_viewport/platform_viewport_android.h"
#include "shell/android/android_handler.h"
#include "shell/android/intent_receiver_manager_impl.h"
#include "shell/android/keyboard_impl.h"
#include "shell/android/main.h"

namespace {

base::android::RegistrationMethod kMojoRegisteredMethods[] = {
    {"AndroidHandler", shell::RegisterAndroidHandlerJni},
    {"IntentReceiverRegistry", shell::RegisterIntentReceiverRegistry},
    {"Keyboard", shell::RegisterKeyboardJni},
    {"PlatformViewportAndroid",
     native_viewport::PlatformViewportAndroid::Register},
    {"ShellMain", shell::RegisterShellMain},
};

bool RegisterJNI(JNIEnv* env) {
  if (!base::android::RegisterJni(env))
    return false;

  if (!mojo::android::RegisterCoreImpl(env))
    return false;

  return RegisterNativeMethods(env, kMojoRegisteredMethods,
                               arraysize(kMojoRegisteredMethods));
}

}  // namespace

// This is called by the VM when the shared library is first loaded.
JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  std::vector<base::android::RegisterCallback> register_callbacks;
  register_callbacks.push_back(base::Bind(&RegisterJNI));
  if (!base::android::OnJNIOnLoadRegisterJNI(vm, register_callbacks) ||
      !base::android::OnJNIOnLoadInit(
          std::vector<base::android::InitCallback>())) {
    return -1;
  }

  return JNI_VERSION_1_4;
}
