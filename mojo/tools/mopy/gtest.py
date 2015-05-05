# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Apptest runner specific to the particular gtest-based apptest framework in
/mojo/public/cpp/application/tests/, built on top of the general apptest
runner."""

import logging
import os
import re
import sys

from mopy.apptest import run_apptest
from mopy.print_process_error import print_process_error


_logger = logging.getLogger()


def _gtest_apptest_output_test(output):
  # Fail on output with gtest's "[  FAILED  ]" or a lack of "[  PASSED  ]".
  # The latter condition ensures failure on broken command lines or output.
  # Check output instead of exit codes because mojo_shell always exits with 0.
  if (output is None or
      (output.find("[  FAILED  ]") != -1 or output.find("[  PASSED  ]") == -1)):
    return False
  return True


def set_color():
  """Run gtests with color if we're on a TTY (and we're not being told
  explicitly what to do)."""
  if sys.stdout.isatty() and "GTEST_COLOR" not in os.environ:
    _logger.debug("Setting GTEST_COLOR=yes")
    os.environ["GTEST_COLOR"] = "yes"


# TODO(vtl): The return value is bizarre. Should just make it either return
# True/False, or a list of failing fixtures. But the dart_apptest runner would
# also need to be updated in the same way.
def run_gtest_apptest(shell, apptest_dict, apptest, isolate, test_args,
                      shell_args):
  """Runs the gtest-based tests in the |apptest| app either altogether or in
  isolation.

  Args:
    shell: Wrapper around concrete Mojo shell, implementing devtools Shell
        interface.
    apptest: Url of the test app.
    isolate: Iff True, each test in the app will be run in a separate shell run.
    test_args: Arguments for the test app.
    shell_args: The arguments for mojo_shell.
  """

  if not isolate:
    if not run_apptest(shell, shell_args, {apptest: test_args},
                       _gtest_apptest_output_test):
      return "Failed test(s) in %r" % apptest_dict
    return "Succeeded"

  # List the apptest fixtures so they can be run independently for isolation.
  fixtures = get_fixtures(shell, shell_args, apptest)

  if not fixtures:
    return "Failed with no tests found."

  apptest_result = "Succeeded"
  for fixture in fixtures:
    isolated_test_args = test_args + ["--gtest_filter=%s" % fixture]
    success = run_apptest(shell, shell_args, {apptest: isolated_test_args},
                          _gtest_apptest_output_test)

    if not success:
      apptest_result = "Failed test(s) in %r" % apptest_dict

  return apptest_result


def get_fixtures(shell, shell_args, apptest):
  """Returns the "Test.Fixture" list from an apptest using mojo_shell.

  Tests are listed by running the given apptest in mojo_shell and passing
  --gtest_list_tests. The output is parsed and reformatted into a list like
  [TestSuite.TestFixture, ... ]
  An empty list is returned on failure, with errors logged.

  Args:
    apptest: The URL of the test application to run.
  """
  arguments = []
  arguments.extend(shell_args)
  arguments.append("--args-for=%s %s" % (apptest, "--gtest_list_tests"))
  arguments.append(apptest)

  (exit_code, output) = shell.RunUntilCompletion(arguments)
  if exit_code:
    print "Failed to get test fixtures:"
    command_line = "mojo_shell " + " ".join(["%r" % x for x in arguments])
    print_process_error(command_line, output)
    return []

  _logger.debug("Tests listed:\n%s" % output)
  return _gtest_list_tests(output)


def _gtest_list_tests(gtest_list_tests_output):
  """Returns a list of strings formatted as TestSuite.TestFixture from the
  output of running --gtest_list_tests on a GTEST application."""

  # Remove log lines.
  gtest_list_tests_output = re.sub("^(\[|WARNING: linker:).*\n",
                                   "",
                                   gtest_list_tests_output,
                                   flags=re.MULTILINE)

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
