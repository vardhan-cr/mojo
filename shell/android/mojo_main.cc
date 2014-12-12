// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/mojo_main.h"

#include "base/android/command_line_android.h"
#include "base/android/java_handler_thread.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/at_exit.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "jni/MojoMain_jni.h"
#include "mojo/application_manager/application_loader.h"
#include "mojo/application_manager/application_manager.h"
#include "shell/context.h"
#include "shell/init.h"
#include "ui/gl/gl_surface_egl.h"

using base::LazyInstance;

namespace mojo {

namespace {

LazyInstance<scoped_ptr<base::MessageLoop>> g_java_message_loop =
    LAZY_INSTANCE_INITIALIZER;

LazyInstance<scoped_ptr<shell::Context>> g_context = LAZY_INSTANCE_INITIALIZER;

LazyInstance<scoped_ptr<base::android::JavaHandlerThread>> g_shell_thread =
    LAZY_INSTANCE_INITIALIZER;

void RunShell(std::vector<GURL> app_urls) {
  shell::Context* context = g_context.Pointer()->get();
  context->Init();
  context->set_ui_loop(g_java_message_loop.Get().get());
  for (std::vector<GURL>::const_iterator it = app_urls.begin();
       it != app_urls.end(); ++it) {
    context->Run(*it);
  }
}

}  // namespace

static void Init(JNIEnv* env,
                 jclass clazz,
                 jobject context,
                 jstring mojo_shell_path,
                 jobjectArray jparameters,
                 jstring j_local_apps_directory) {
  base::android::ScopedJavaLocalRef<jobject> scoped_context(env, context);
  base::android::InitApplicationContext(env, scoped_context);

  base::android::InitNativeCommandLineFromJavaArray(env, jparameters);
  base::FilePath mojo_shell_file_path(
      base::android::ConvertJavaStringToUTF8(env, mojo_shell_path));
  base::CommandLine::ForCurrentProcess()->SetProgram(mojo_shell_file_path);
  mojo::shell::InitializeLogging();

  // We want ~MessageLoop to happen prior to ~Context. Initializing
  // LazyInstances is akin to stack-allocating objects; their destructors
  // will be invoked first-in-last-out.
  shell::Context* shell_context = new shell::Context();
  shell_context->mojo_url_resolver()->SetLocalAppsPath(base::FilePath(
      base::android::ConvertJavaStringToUTF8(env, j_local_apps_directory)));
  g_context.Get().reset(shell_context);

  g_java_message_loop.Get().reset(new base::MessageLoopForUI);
  base::MessageLoopForUI::current()->Start();

  // TODO(abarth): At which point should we switch to cross-platform
  // initialization?

  gfx::GLSurface::InitializeOneOff();
}

static void Start(JNIEnv* env, jclass clazz, jstring jurl) {
  std::vector<GURL> app_urls;
#if defined(MOJO_SHELL_DEBUG_URL)
  app_urls.push_back(GURL(MOJO_SHELL_DEBUG_URL));
  // Sleep for 5 seconds to give the debugger a chance to attach.
  sleep(5);
#else
  if (jurl)
    app_urls.push_back(GURL(base::android::ConvertJavaStringToUTF8(env, jurl)));
#endif

  g_shell_thread.Get().reset(
      new base::android::JavaHandlerThread("shell_thread"));
  g_shell_thread.Get()->Start();
  g_shell_thread.Get()->message_loop()->PostTask(
      FROM_HERE, base::Bind(&RunShell, app_urls));
}

bool RegisterMojoMain(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace mojo
