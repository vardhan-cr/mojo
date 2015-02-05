// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/out_of_process_dynamic_service_runner.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/scoped_native_library.h"

namespace mojo {
namespace shell {

OutOfProcessDynamicServiceRunner::OutOfProcessDynamicServiceRunner(
    Context* context)
    : context_(context) {
}

OutOfProcessDynamicServiceRunner::~OutOfProcessDynamicServiceRunner() {
  if (app_child_process_host_) {
    // TODO(vtl): Race condition: If |AppChildProcessHost::DidStart()| hasn't
    // been called yet, we shouldn't call |Join()| here. (Until |DidStart()|, we
    // may not have a child process to wait on.) Probably we should fix
    // |Join()|.
    app_child_process_host_->Join();
  }
}

void OutOfProcessDynamicServiceRunner::Start(
    const base::FilePath& app_path,
    DynamicServiceRunner::CleanupBehavior cleanup_behavior,
    InterfaceRequest<Application> application_request,
    const base::Closure& app_completed_callback) {
  app_path_ = app_path;

  DCHECK(app_completed_callback_.is_null());
  app_completed_callback_ = app_completed_callback;

  app_child_process_host_.reset(
      new AppChildProcessHost(context_));
  app_child_process_host_->Start();

  // TODO(vtl): |app_path.AsUTF8Unsafe()| is unsafe.
  app_child_process_host_->StartApp(
      app_path.AsUTF8Unsafe(), cleanup_behavior == DeleteAppPath,
      application_request.Pass(),
      base::Bind(&OutOfProcessDynamicServiceRunner::AppCompleted,
                 base::Unretained(this)));
}

void OutOfProcessDynamicServiceRunner::AppCompleted(int32_t result) {
  DVLOG(2) << "OutOfProcessDynamicServiceRunner::AppCompleted(" << result
           << ")";

  app_child_process_host_.reset();
  // This object may be deleted by this callback.
  base::Closure app_completed_callback = app_completed_callback_;
  app_completed_callback_.Reset();
  app_completed_callback.Run();
}

}  // namespace shell
}  // namespace mojo
