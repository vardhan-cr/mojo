#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test runner for gtest application tests."""

import logging
import os
import sys

_logging = logging.getLogger()

from mopy.config import Config
import mopy.gtest
from mopy.paths import Paths


def main(argv):
  logging.basicConfig()
  # Uncomment to debug:
  # _logging.setLevel(logging.DEBUG)

  if len(argv) != 3:
    print "Usage: %s gtest_app_list_file root_dir" % os.path.basename(argv[0])
    return 0

  _logging.debug("Test list file: %s", argv[1])
  with open(argv[1], 'rb') as f:
    apptest_list = [y for y in [x.strip() for x in f.readlines()] \
                        if y and y[0] != '#']
  _logging.debug("Test list: %s" % apptest_list)

  # Run gtests with color if we're on a TTY (and we're not being told explicitly
  # what to do).
  if sys.stdout.isatty() and 'GTEST_COLOR' not in os.environ:
    _logging.debug("Setting GTEST_COLOR=yes")
    os.environ['GTEST_COLOR'] = 'yes'

  mojo_shell = Paths(Config(build_dir=argv[2])).mojo_shell_path

  exit_code = 0
  for apptest in apptest_list:
    print "Running " + apptest + "...",
    sys.stdout.flush()

    # List the apptest fixtures so they can be run independently for isolation.
    # TODO(msw): Run some apptests without fixture isolation?
    fixtures = mopy.gtest.get_fixtures(mojo_shell, apptest)

    if not fixtures:
      print "Failed with no tests found."
      exit_code = 1
      continue

    apptest_result = "Succeeded"
    for fixture in fixtures:
      # ExampleApplicationTest.CheckCommandLineArg checks --example_apptest_arg.
      # TODO(msw): Enable passing arguments to tests down from the test harness.
      command = [mojo_shell,
                 "--args-for={0} --example_apptest_arg --gtest_filter={1}"
                     .format(apptest, fixture),
                 "--args-for=mojo:native_viewport_service "
                     "--use-headless-config --use-osmesa",
                 apptest]
      if not mopy.gtest.run_test(command):
        apptest_result = "Failed test(s) in " + apptest
        exit_code = 1
    print apptest_result

  return exit_code

if __name__ == '__main__':
  sys.exit(main(sys.argv))
