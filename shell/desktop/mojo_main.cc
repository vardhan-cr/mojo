// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <iostream>

#include "base/at_exit.h"
#include "base/base_switches.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "shell/child_process.h"
#include "shell/command_line_util.h"
#include "shell/context.h"
#include "shell/init.h"
#include "shell/switches.h"

namespace {

void Usage() {
  std::cerr << "Launch Mojo applications.\n";
  std::cerr
      << "Usage: mojo_shell"
      << " [--" << switches::kArgsFor << "=<mojo-app>]"
      << " [--" << switches::kContentHandlers << "=<handlers>]"
      << " [--" << switches::kEnableExternalApplications << "]"
      << " [--" << switches::kDisableCache << "]"
      << " [--" << switches::kEnableMultiprocess << "]"
      << " [--" << switches::kOrigin << "=<url-lib-path>]"
      << " [--" << switches::kURLMappings << "=from1=to1,from2=to2]"
      << " [--" << switches::kPredictableAppFilenames << "]"
      << " [--" << switches::kWaitForDebugger << "]"
      << " <mojo-app> ...\n\n"
      << "A <mojo-app> is a Mojo URL or a Mojo URL and arguments within "
      << "quotes.\n"
      << "Example: mojo_shell \"mojo:js_standalone test.js\".\n"
      << "<url-lib-path> is searched for shared libraries named by mojo URLs.\n"
      << "The value of <handlers> is a comma separated list like:\n"
      << "text/html,mojo:html_viewer,"
      << "application/javascript,mojo:js_content_handler\n";
}

}  // namespace

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

  mojo::shell::InitializeLogging();

  // TODO(vtl): Unify parent and child process cases to the extent possible.
  if (scoped_ptr<mojo::shell::ChildProcess> child_process =
          mojo::shell::ChildProcess::Create(
              *base::CommandLine::ForCurrentProcess())) {
    child_process->Main();
  } else {
    // Only check the command line for the main process.
    const base::CommandLine& command_line =
        *base::CommandLine::ForCurrentProcess();

    const std::set<std::string> all_switches = switches::GetAllSwitches();
    const base::CommandLine::SwitchMap switches = command_line.GetSwitches();
    bool found_unknown_switch = false;
    for (const auto& s : switches) {
      if (all_switches.find(s.first) == all_switches.end()) {
        std::cerr << "unknown switch: " << s.first << std::endl;
        found_unknown_switch = true;
      }
    }

    if (found_unknown_switch ||
        (!command_line.HasSwitch(switches::kEnableExternalApplications) &&
         (command_line.HasSwitch(switches::kHelp) ||
          command_line.GetArgs().empty()))) {
      Usage();
      return 0;
    }

    // We want the shell::Context to outlive the MessageLoop so that pipes are
    // all gracefully closed / error-out before we try to shut the Context down.
    mojo::shell::Context shell_context;
    {
      base::MessageLoop message_loop;
      if (!shell_context.Init()) {
        Usage();
        return 0;
      }

      // The mojo_shell --args-for command-line switch is handled specially
      // because it can appear more than once. The base::CommandLine class
      // collapses multiple occurrences of the same switch.
      for (int i = 1; i < argc; i++) {
        ApplyApplicationArgs(&shell_context, argv[i]);
      }

      message_loop.PostTask(
          FROM_HERE,
          base::Bind(&mojo::shell::RunCommandLineApps, &shell_context));
      message_loop.Run();

      // Must be called before |message_loop| is destroyed.
      shell_context.Shutdown();
    }
  }
  return 0;
}
