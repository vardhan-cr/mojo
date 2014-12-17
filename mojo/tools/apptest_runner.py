#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test runner for gtest application tests."""

import argparse
import ast
import logging
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

  parser.add_argument('apptest_list_file', type=file)
  parser.add_argument('build_dir', type=str)
  args = parser.parse_args()

  _logging.debug("Test list file: %s", args.apptest_list_file)
  apptest_list = ast.literal_eval(args.apptest_list_file.read())
  _logging.debug("Test list: %s" % apptest_list)

  mopy.gtest.set_color()
  mojo_shell = Paths(build_dir=args.build_dir).mojo_shell_path

  exit_code = 0
  for apptest_dict in apptest_list:
    if apptest_dict.get("disabled"):
      continue

    apptest = apptest_dict["test"]
    apptest_args = apptest_dict.get("test-args", [])
    shell_args = apptest_dict.get("shell-args", [])
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
      args_for_apptest = " ".join(["--args-for=" + apptest,
                                   "--gtest_filter=" + fixture] + apptest_args)
      command = [mojo_shell, apptest, args_for_apptest] + shell_args

      if not mopy.gtest.run_test(command):
        apptest_result = "Failed test(s) in " + apptest
        exit_code = 1
    print apptest_result

  return exit_code

if __name__ == '__main__':
  sys.exit(main())
