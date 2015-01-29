# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import subprocess
import sys

_logging = logging.getLogger()

from mopy import android
from mopy.config import Config
from mopy.paths import Paths
from mopy.print_process_error import print_process_error


def set_color():
  """Run gtests with color if we're on a TTY (and we're not being told
  explicitly what to do)."""
  if sys.stdout.isatty() and "GTEST_COLOR" not in os.environ:
    _logging.debug("Setting GTEST_COLOR=yes")
    os.environ["GTEST_COLOR"] = "yes"


def run_test(config, shell_args, apps_and_args=None, context=None,
             run_launcher=False):
  """Runs a command line and checks the output for signs of gtest failure.

  Args:
    config: The mopy.config.Config object for the build.
    shell_args: The arguments for mojo_shell.
    apps_and_args: A Dict keyed by application URL associated to the
        application's specific arguments.
    context: Platform specific context. See |mopy.{platform}.Context|.
    run_launcher: |True| is mojo_launcher must be used instead of mojo_shell.
  """
  apps_and_args = apps_and_args or {}
  output = _try_run_test(
      config, shell_args, apps_and_args, context, run_launcher)
  # Fail on output with gtest's "[  FAILED  ]" or a lack of "[  PASSED  ]".
  # The latter condition ensures failure on broken command lines or output.
  # Check output instead of exit codes because mojo_shell always exits with 0.
  if (output is None or
      (output.find("[  FAILED  ]") != -1 or output.find("[  PASSED  ]") == -1)):
    print "Failed test:"
    print_process_error(
        _build_command_line(config, shell_args, apps_and_args, run_launcher),
        output)
    return False
  _logging.debug("Succeeded with output:\n%s" % output)
  return True


def get_fixtures(config, apptest, context):
  """Returns the "Test.Fixture" list from an apptest using mojo_shell.

  Tests are listed by running the given apptest in mojo_shell and passing
  --gtest_list_tests. The output is parsed and reformatted into a list like
  [TestSuite.TestFixture, ... ]
  An empty list is returned on failure, with errors logged.

  Args:
    config: The mopy.config.Config object for the build.
    apptest: The URL of the test application to run.
    context: Platform specific context. See |mopy.{platform}.Context|.
  """

  try:
    apps_and_args = {apptest: ["--gtest_list_tests"]}
    list_output = _run_test(config, [], apps_and_args, context)
    _logging.debug("Tests listed:\n%s" % list_output)
    return _gtest_list_tests(list_output)
  except Exception as e:
    print "Failed to get test fixtures:"
    print_process_error(_build_command_line(config, [], apps_and_args), e)
  return []


def build_shell_arguments(shell_args, apps_and_args=None):
  """Build the list of arguments for the shell. |shell_args| are the base
  arguments, |apps_and_args| is a dictionary that associates each application to
  its specific arguments|. Each app included will be run by the shell.
  """
  result = shell_args[:]
  if apps_and_args:
    for (application, args) in apps_and_args.items():
      result += ["--args-for=%s %s" % (application, " ".join(args))]
    result += apps_and_args.keys()
  return result


def _get_shell_executable(config, run_launcher):
  paths = Paths(config=config)
  if config.target_os == Config.OS_ANDROID:
    return os.path.join(paths.src_root, "mojo", "tools",
                        "android_mojo_shell.py")
  elif run_launcher:
    return paths.mojo_launcher_path
  else:
    return paths.mojo_shell_path


def _build_command_line(config, shell_args, apps_and_args, run_launcher=False):
  executable = _get_shell_executable(config, run_launcher)
  return "%s %s" % (executable, " ".join(["%r" % x for x in
                                          build_shell_arguments(
                                              shell_args, apps_and_args)]))


def _run_test_android(shell_args, apps_and_args, context):
  """Run the given test on the android device defined in |context|."""
  (r, w) = os.pipe()
  with os.fdopen(r, "r") as rf:
    with os.fdopen(w, "w") as wf:
      arguments = build_shell_arguments(shell_args, apps_and_args)
      android.StartShell(context, arguments, wf, wf.close)
      return rf.read()


def _run_test(config, shell_args, apps_and_args, context, run_launcher=False):
  """Run the given test, using mojo_launcher if |run_launcher| is True."""
  if (config.target_os == Config.OS_ANDROID):
    return _run_test_android(shell_args, apps_and_args, context)
  else:
    executable = _get_shell_executable(config, run_launcher)
    command = ([executable] + build_shell_arguments(shell_args, apps_and_args))
    return subprocess.check_output(command, stderr=subprocess.STDOUT)


def _try_run_test(config, shell_args, apps_and_args, context, run_launcher):
  """Returns the output of a command line or an empty string on error."""
  command_line = _build_command_line(config, shell_args, apps_and_args,
                                     run_launcher=run_launcher)
  _logging.debug("Running command line: %s" % command_line)
  try:
    return _run_test(config, shell_args, apps_and_args, context, run_launcher)
  except Exception as e:
    print_process_error(command_line, e)
  return None


def _gtest_list_tests(gtest_list_tests_output):
  """Returns a list of strings formatted as TestSuite.TestFixture from the
  output of running --gtest_list_tests on a GTEST application."""

  # Remove log lines.
  gtest_list_tests_output = (
      re.sub("^\[.*\n", "", gtest_list_tests_output, flags=re.MULTILINE))

  if not re.match("^(\w*\.\r?\n(  \w*\r?\n)+)+", gtest_list_tests_output):
    raise Exception("Unrecognized --gtest_list_tests output:\n%s" %
                    gtest_list_tests_output)

  output_lines = gtest_list_tests_output.split("\n")

  test_list = []
  for line in output_lines:
    if not line:
      continue
    if line[0] != " ":
      suite = line.strip()
      continue
    test_list.append(suite + line.strip())

  return test_list
