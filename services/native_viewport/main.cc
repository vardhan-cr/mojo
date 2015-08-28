// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#if defined(USE_OZONE)
#include "services/native_viewport/ozone/app_delegate_ozone.h"
#else
#include "services/native_viewport/app_delegate.h"
#endif

MojoResult MojoMain(MojoHandle application_request) {
#if defined(USE_OZONE)
  mojo::ApplicationRunnerChromium runner(
      new native_viewport::NativeViewportOzoneAppDelegate);
#else
  mojo::ApplicationRunnerChromium runner(
      new native_viewport::NativeViewportAppDelegate);
#endif
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(application_request);
}
