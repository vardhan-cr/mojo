// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "services/reaper/reaper_impl.h"

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new reaper::ReaperImpl());
  return runner.Run(shell_handle);
}
