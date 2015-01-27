// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "shell/external_application_registrar_connection.h"
#include "shell/in_process_dynamic_service_runner.h"
#include "shell/init.h"
#include "url/gurl.h"

namespace {
const char kAppPath[] = "app-path";
const char kAppURL[] = "app-url";
const char kShellPath[] = "shell-path";
}

class Launcher {
 public:
  explicit Launcher(base::CommandLine* command_line)
      : app_path_(command_line->GetSwitchValuePath(kAppPath)),
        app_url_(command_line->GetSwitchValueASCII(kAppURL)),
        app_args_(command_line->GetArgs()),
        loop_(base::MessageLoop::TYPE_IO),
        connection_(
            base::FilePath(command_line->GetSwitchValuePath(kShellPath))) {}
  ~Launcher() {}

  bool Connect() { return connection_.Connect(); }

  bool Register() {
    DCHECK(!run_loop_.get());
    run_loop_.reset(new base::RunLoop);
    connection_.Register(
        app_url_, app_args_,
        base::Bind(&Launcher::OnRegistered, base::Unretained(this)));
    run_loop_->Run();
    run_loop_.reset();
    return application_request_.is_pending();
  }

  void Run() {
    DCHECK(!run_loop_.get());
    DCHECK(application_request_.is_pending());
    mojo::shell::InProcessDynamicServiceRunner service_runner(nullptr);
    run_loop_.reset(new base::RunLoop);
    service_runner.Start(
        app_path_, application_request_.Pass(),
        base::Bind(&Launcher::OnAppCompleted, base::Unretained(this)));
    run_loop_->Run();
    run_loop_.reset();
  }

 private:
  void OnRegistered(
      mojo::InterfaceRequest<mojo::Application> application_request) {
    application_request_ = application_request.Pass();
    run_loop_->Quit();
  }

  void OnAppCompleted() { run_loop_->Quit(); }

  const base::FilePath app_path_;
  const GURL app_url_;
  std::vector<std::string> app_args_;
  base::MessageLoop loop_;
  mojo::shell::ExternalApplicationRegistrarConnection connection_;
  mojo::InterfaceRequest<mojo::Application> application_request_;
  scoped_ptr<base::RunLoop> run_loop_;
};

#if defined(OS_WIN)
int main(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
  base::AtExitManager at_exit;
  mojo::embedder::Init(scoped_ptr<mojo::embedder::PlatformSupport>(
      new mojo::embedder::SimplePlatformSupport()));

  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  mojo::shell::InitializeLogging();

  Launcher launcher(command_line);
  if (!launcher.Connect()) {
    LOG(ERROR) << "Failed to connect on socket "
               << command_line->GetSwitchValueASCII(kShellPath);
    return MOJO_RESULT_INVALID_ARGUMENT;
  }

  if (!launcher.Register()) {
    LOG(ERROR) << "Error registering "
               << command_line->GetSwitchValueASCII(kAppURL);
    return MOJO_RESULT_INVALID_ARGUMENT;
  }

  launcher.Run();
  return MOJO_RESULT_OK;
}
