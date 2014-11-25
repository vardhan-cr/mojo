// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/shell/android/android_handler.h"

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "jni/AndroidHandler_jni.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"

using base::FilePath;
using base::File;
using base::android::AttachCurrentThread;
using base::android::ScopedJavaLocalRef;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF8ToJavaString;
using base::android::GetApplicationContext;

namespace mojo {

namespace {}  // namespace

void AndroidHandler::RunApplication(ShellPtr shell, URLResponsePtr response) {
  JNIEnv* env = AttachCurrentThread();
  ScopedJavaLocalRef<jstring> j_archive_path =
      Java_AndroidHandler_getNewTempArchivePath(env, GetApplicationContext());
  FilePath archive_path(ConvertJavaStringToUTF8(env, j_archive_path.obj()));

  mojo::common::BlockingCopyToFile(response->body.Pass(), archive_path);
  Java_AndroidHandler_bootstrap(env, GetApplicationContext(),
                                j_archive_path.obj(),
                                shell.PassMessagePipe().release().value());
}

void AndroidHandler::Initialize(ApplicationImpl* app) {
}

bool AndroidHandler::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  connection->AddService(&content_handler_factory_);
  return true;
}

bool RegisterAndroidHandlerJni(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace mojo
