# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Apptest runner specific to the particular Dart apptest framework in
/mojo/dart/apptests, built on top of the general apptest runner."""

import logging

_logging = logging.getLogger()

from mopy.apptest import run_apptest


def _dart_apptest_output_test(output):
  # Fail on output with dart unittests' "FAIL:"/"ERROR:" or a lack of "PASS:".
  # The latter condition ensures failure on broken command lines or output.
  # Check output instead of exit codes because mojo_shell always exits with 0.
  if (not output or
      '\nFAIL: ' in output or
      '\nERROR: ' in output or
      '\nPASS: ' not in output):
    return False
  return True


# TODO(erg): Support android, launched services and fixture isolation.
def run_dart_apptest(shell, apptest_dict, shell_args, apps_and_args=None):
  """Runs a command line and checks the output for signs of gtest failure.

  Args:
    shell_args: The arguments for mojo_shell.
    apps_and_args: A Dict keyed by application URL associated to the
        application's specific arguments.
  """
  apps_and_args = apps_and_args or {}
  if not run_apptest(shell, shell_args, apps_and_args,
                     _dart_apptest_output_test):
    return "Failed test(s) in %r" % apptest_dict
  return "Succeeded"
