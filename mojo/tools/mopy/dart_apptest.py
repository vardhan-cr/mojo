# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

_logging = logging.getLogger()

from mopy import test_util
from mopy.print_process_error import print_process_error


# TODO(erg): Support android, launched services and fixture isolation.
def run_test(config, apptest_dict, shell_args, apps_and_args=None,
             run_launcher=False):
  """Runs a command line and checks the output for signs of gtest failure.

  Args:
    config: The mopy.config.Config object for the build.
    shell_args: The arguments for mojo_shell.
    apps_and_args: A Dict keyed by application URL associated to the
        application's specific arguments.
    run_launcher: |True| is mojo_launcher must be used instead of mojo_shell.
  """
  apps_and_args = apps_and_args or {}
  output = test_util.try_run_test(config, shell_args, apps_and_args,
                                  run_launcher)
  # Fail on output with dart unittests' "FAIL:" or a lack of "PASS:".
  # The latter condition ensures failure on broken command lines or output.
  # Check output instead of exit codes because mojo_shell always exits with 0.
  if (output is None or
      output.find("FAIL:") != -1 or
      output.find("PASS:") == -1):
    print "Failed test:"
    print_process_error(
        test_util.build_command_line(config, shell_args, apps_and_args,
                                     run_launcher),
        output)
    return "Failed test(s) in %r" % apptest_dict
  _logging.debug("Succeeded with output:\n%s" % output)
  return "Succeeded"

