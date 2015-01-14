// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/base/android/ui_base_jni_registrar.h"

#include "base/android/jni_android.h"
#include "base/android/jni_registrar.h"
#include "ui/base/android/view_android.h"
#include "ui/base/android/window_android.h"
#include "ui/base/touch/touch_device.h"

namespace ui {
namespace android {

static base::android::RegistrationMethod kUiRegisteredMethods[] = {
  { "TouchDevice", RegisterTouchDeviceAndroid },
  { "ViewAndroid", ViewAndroid::RegisterViewAndroid },
  { "WindowAndroid", WindowAndroid::RegisterWindowAndroid },
};

bool RegisterJni(JNIEnv* env) {
  return RegisterNativeMethods(env, kUiRegisteredMethods,
                               arraysize(kUiRegisteredMethods));
}

}  // namespace android
}  // namespace ui
