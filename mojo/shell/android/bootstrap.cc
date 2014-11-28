// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/scoped_native_library.h"
#include "jni/Bootstrap_jni.h"
#include "mojo/shell/dynamic_service_runner.h"

namespace mojo {

namespace {
// Most applications will need to access the Android ApplicationContext in which
// they are run. If the application library exports the InitApplicationContext
// function, we will set it there.
void SetApplicationContextIfNeeded(
    JNIEnv* env,
    const base::ScopedNativeLibrary& app_library,
    jobject context) {
  const char* init_application_context_name = "InitApplicationContext";
  typedef void (*InitApplicationContextFn)(
      const base::android::JavaRef<jobject>&);
  InitApplicationContextFn init_application_context =
      reinterpret_cast<InitApplicationContextFn>(
          app_library.GetFunctionPointer(init_application_context_name));
  if (init_application_context) {
    base::android::ScopedJavaLocalRef<jobject> scoped_context(env, context);
    init_application_context(scoped_context);
  }
}

}  // namespace

void Bootstrap(JNIEnv* env,
               jobject,
               jobject j_context,
               jstring j_native_library_path,
               jint j_handle) {
  base::FilePath app_path(
      base::android::ConvertJavaStringToUTF8(env, j_native_library_path));
  ScopedMessagePipeHandle handle((mojo::MessagePipeHandle(j_handle)));

  // Load the library, so that we can set the application context there if
  // needed.
  base::NativeLibraryLoadError error;
  base::ScopedNativeLibrary app_library(
      base::LoadNativeLibrary(app_path, &error));
  if (!app_library.is_valid()) {
    LOG(ERROR) << "Failed to load app library (error: " << error.ToString()
               << ")";
    return;
  }
  SetApplicationContextIfNeeded(env, app_library, j_context);

  // Run the application.
  base::ScopedNativeLibrary app_library_from_runner(
      shell::DynamicServiceRunner::LoadAndRunService(app_path, handle.Pass()));
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
