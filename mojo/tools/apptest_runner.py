#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test runner for gtest application tests."""

import argparse
import logging
import os
import sys

_logging = logging.getLogger()

import mopy.gtest
from mopy.paths import Paths


def main():
  logging.basicConfig()
  # Uncomment to debug:
  # _logging.setLevel(logging.DEBUG)

  parser = argparse.ArgumentParser(description='A test runner for gtest '
                                   'application tests.')

  parser.add_argument('apptests', type=file, metavar='gtest_app_list_file')
  parser.add_argument('build_dir', type=str)
  args = parser.parse_args()

  apptest_list = [y.rstrip() for y in args.apptests \
                      if not y.strip().startswith('#')]
  _logging.debug("Test list: %s" % apptest_list)

  mopy.gtest.set_color()
  mojo_shell = Paths(build_dir=args.build_dir).mojo_shell_path

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
  sys.exit(main())
