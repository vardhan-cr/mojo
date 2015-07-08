// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/child_process_host.h"

#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/process/kill.h"
#include "base/process/launch.h"
#include "base/task_runner.h"
#include "base/task_runner_util.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "shell/child_switches.h"
#include "shell/context.h"
#include "shell/task_runners.h"

namespace shell {

struct ChildProcessHost::LaunchData {
  LaunchData() {}
  ~LaunchData() {}

  base::FilePath child_path;
  mojo::embedder::PlatformChannelPair platform_channel_pair;
  std::string child_connection_id;
};

ChildProcessHost::ChildProcessHost(Context* context)
    : context_(context), channel_info_(nullptr) {
}

ChildProcessHost::~ChildProcessHost() {
  DCHECK(!child_process_.IsValid());
}

void ChildProcessHost::Start() {
  DCHECK(!child_process_.IsValid());

  scoped_ptr<LaunchData> launch_data(new LaunchData());
  launch_data->child_path = context_->mojo_shell_child_path();

  // TODO(vtl): Add something for |slave_info|.
  // TODO(vtl): The "unretained this" is wrong (see also below).
  mojo::ScopedMessagePipeHandle handle(mojo::embedder::ConnectToSlave(
      nullptr, launch_data->platform_channel_pair.PassServerHandle(),
      base::Bind(&ChildProcessHost::DidConnectToSlave, base::Unretained(this)),
      base::MessageLoop::current()->message_loop_proxy(),
      &launch_data->child_connection_id, &channel_info_));
  // TODO(vtl): We should destroy the channel on destruction (using
  // |channel_info_|, but only after the callback has been called.
  CHECK(channel_info_);

  controller_.Bind(mojo::InterfacePtrInfo<ChildController>(handle.Pass(), 0u));
  controller_.set_connection_error_handler([this]() { OnConnectionError(); });

  CHECK(base::PostTaskAndReplyWithResult(
      context_->task_runners()->blocking_pool(), FROM_HERE,
      base::Bind(&ChildProcessHost::DoLaunch, base::Unretained(this),
                 base::Passed(&launch_data)),
      base::Bind(&ChildProcessHost::DidStart, base::Unretained(this))));
}

int ChildProcessHost::Join() {
  DCHECK(child_process_.IsValid());
  int rv = -1;
  LOG_IF(ERROR, !child_process_.WaitForExit(&rv))
      << "Failed to wait for child process";
  child_process_.Close();
  return rv;
}

void ChildProcessHost::StartApp(
    const mojo::String& app_path,
    mojo::InterfaceRequest<mojo::Application> application_request,
    const ChildController::StartAppCallback& on_app_complete) {
  DCHECK(controller_);

  on_app_complete_ = on_app_complete;
  controller_->StartApp(
      app_path, application_request.Pass(),
      base::Bind(&ChildProcessHost::AppCompleted, base::Unretained(this)));
}

void ChildProcessHost::ExitNow(int32_t exit_code) {
  DCHECK(controller_);

  controller_->ExitNow(exit_code);
}

void ChildProcessHost::DidStart(base::Process child_process) {
  DVLOG(2) << "ChildProcessHost::DidStart()";
  DCHECK(!child_process_.IsValid());

  if (!child_process.IsValid()) {
    LOG(ERROR) << "Failed to start app child process";
    AppCompleted(MOJO_RESULT_UNKNOWN);
    return;
  }

  child_process_ = child_process.Pass();
}

// Callback for |mojo::embedder::ConnectToSlave()|.
void ChildProcessHost::DidConnectToSlave() {
  DVLOG(2) << "ChildProcessHost::DidConnectToSlave()";
}

base::Process ChildProcessHost::DoLaunch(scoped_ptr<LaunchData> launch_data) {
  static const char* kForwardSwitches[] = {
      switches::kTraceToConsole, switches::kV, switches::kVModule,
  };

  base::CommandLine child_command_line(launch_data->child_path);
  child_command_line.CopySwitchesFrom(*base::CommandLine::ForCurrentProcess(),
                                      kForwardSwitches,
                                      arraysize(kForwardSwitches));
  child_command_line.AppendSwitchASCII(switches::kChildConnectionId,
                                       launch_data->child_connection_id);

  mojo::embedder::HandlePassingInformation handle_passing_info;
  launch_data->platform_channel_pair.PrepareToPassClientHandleToChildProcess(
      &child_command_line, &handle_passing_info);

  base::LaunchOptions options;
  options.fds_to_remap = &handle_passing_info;
  DVLOG(2) << "Launching child with command line: "
           << child_command_line.GetCommandLineString();
  base::Process child_process =
      base::LaunchProcess(child_command_line, options);
  if (child_process.IsValid())
    launch_data->platform_channel_pair.ChildProcessLaunched();
  return child_process.Pass();
}

void ChildProcessHost::AppCompleted(int32_t result) {
  if (!on_app_complete_.is_null()) {
    auto on_app_complete = on_app_complete_;
    on_app_complete_.reset();
    on_app_complete.Run(result);
  }
}

void ChildProcessHost::OnConnectionError() {
  AppCompleted(MOJO_RESULT_UNKNOWN);
}

}  // namespace shell
