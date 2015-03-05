// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/in_process_native_runner.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/threading/platform_thread.h"
#include "shell/dynamic_service_runner.h"

namespace mojo {
namespace shell {

InProcessNativeRunner::InProcessNativeRunner(Context* context)
    : cleanup_behavior_(NativeRunner::DontDeleteAppPath),
      app_library_(nullptr) {
}

InProcessNativeRunner::~InProcessNativeRunner() {
  // It is important to let the thread exit before unloading the DSO (when
  // app_library_ is destructed), because the library may have registered
  // thread-local data and destructors to run on thread termination.
  if (thread_) {
    DCHECK(thread_->HasBeenStarted());
    DCHECK(!thread_->HasBeenJoined());
    thread_->Join();
  }
}

void InProcessNativeRunner::Start(
    const base::FilePath& app_path,
    NativeRunner::CleanupBehavior cleanup_behavior,
    InterfaceRequest<Application> application_request,
    const base::Closure& app_completed_callback) {
  app_path_ = app_path;
  cleanup_behavior_ = cleanup_behavior;

  DCHECK(!application_request_.is_pending());
  application_request_ = application_request.Pass();

  DCHECK(app_completed_callback_runner_.is_null());
  app_completed_callback_runner_ =
      base::Bind(&base::TaskRunner::PostTask, base::MessageLoopProxy::current(),
                 FROM_HERE, app_completed_callback);

  DCHECK(!thread_);
  thread_.reset(new base::DelegateSimpleThread(this, "app_thread"));
  thread_->Start();
}

void InProcessNativeRunner::Run() {
  DVLOG(2) << "Loading/running Mojo app in process from library: "
           << app_path_.value()
           << " thread id=" << base::PlatformThread::CurrentId();

  app_library_.Reset(LoadAndRunNativeApplication(app_path_, cleanup_behavior_,
                                                 application_request_.Pass()));
  app_completed_callback_runner_.Run();
  app_completed_callback_runner_.Reset();
}

scoped_ptr<NativeRunner> InProcessNativeRunnerFactory::Create(
    const Options& options) {
  return make_scoped_ptr(new InProcessNativeRunner(context_));
}

}  // namespace shell
}  // namespace mojo
