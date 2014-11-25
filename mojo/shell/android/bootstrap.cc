// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "jni/Bootstrap_jni.h"
#include "mojo/shell/dynamic_service_runner.h"

namespace mojo {

void Bootstrap(JNIEnv* env,
               jobject,
               jstring j_native_library_path,
               jint j_handle) {
  std::string native_library_path =
      base::android::ConvertJavaStringToUTF8(env, j_native_library_path);

  ScopedMessagePipeHandle handle((mojo::MessagePipeHandle(j_handle)));
  shell::DynamicServiceRunner::LoadAndRunService(
      base::FilePath(native_library_path), handle.Pass());
}

bool RegisterBootstrapJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace mojo

JNI_EXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  base::android::InitVM(vm);
  JNIEnv* env = base::android::AttachCurrentThread();

  if (!mojo::RegisterBootstrapJni(env))
    return -1;

  return JNI_VERSION_1_4;
}
