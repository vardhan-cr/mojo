// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_split.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "shell/dynamic_service_runner.h"
#include "shell/external_application_registrar_connection.h"
#include "shell/in_process_dynamic_service_runner.h"
#include "shell/init.h"
#include "url/gurl.h"

namespace {
const char kAppArgs[] = "app-args";
const char kAppPath[] = "app-path";
const char kAppURL[] = "app-url";
const char kShellPath[] = "shell-path";
}

class Launcher {
 public:
  explicit Launcher(base::CommandLine* command_line)
      : app_path_(command_line->GetSwitchValuePath(kAppPath)),
        app_url_(command_line->GetSwitchValueASCII(kAppURL)),
        loop_(base::MessageLoop::TYPE_IO),
        connection_(
            base::FilePath(command_line->GetSwitchValuePath(kShellPath))) {
    base::SplitStringAlongWhitespace(
        command_line->GetSwitchValueASCII(kAppArgs), &app_args_);
  }

  ~Launcher() {}

  bool Connect() { return connection_.Connect(); }

  bool Register() {
    base::RunLoop run_loop;
    connection_.Register(
        app_url_, app_args_,
        base::Bind(&Launcher::OnRegistered, base::Unretained(this),
                   base::Unretained(&run_loop)));
    run_loop.Run();
    return application_request_.is_pending();
  }

  void Run() {
    DCHECK(application_request_.is_pending());
    mojo::shell::InProcessDynamicServiceRunner service_runner(nullptr);
    base::RunLoop run_loop;
    service_runner.Start(app_path_,
                         mojo::shell::DynamicServiceRunner::DontDeleteAppPath,
                         application_request_.Pass(), run_loop.QuitClosure());
    run_loop.Run();
  }

 private:
  void OnRegistered(
      base::RunLoop* run_loop,
      mojo::InterfaceRequest<mojo::Application> application_request) {
    application_request_ = application_request.Pass();
    run_loop->Quit();
  }

  const base::FilePath app_path_;
  const GURL app_url_;
  std::vector<std::string> app_args_;
  base::MessageLoop loop_;
  mojo::shell::ExternalApplicationRegistrarConnection connection_;
  mojo::InterfaceRequest<mojo::Application> application_request_;

  DISALLOW_COPY_AND_ASSIGN(Launcher);
};

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  mojo::embedder::Init(
      make_scoped_ptr(new mojo::embedder::SimplePlatformSupport()));

  base::CommandLine::Init(argc, argv);
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  mojo::shell::InitializeLogging();

  Launcher launcher(command_line);
  if (!launcher.Connect()) {
    LOG(ERROR) << "Failed to connect on socket "
               << command_line->GetSwitchValueASCII(kShellPath);
    return 1;
  }

  if (!launcher.Register()) {
    LOG(ERROR) << "Error registering "
               << command_line->GetSwitchValueASCII(kAppURL);
    return 1;
  }

  launcher.Run();
  return 0;
}
