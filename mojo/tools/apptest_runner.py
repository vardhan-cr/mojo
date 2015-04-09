#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test runner for gtest application tests."""

import argparse
import ast
import logging
import sys

from mopy import android
from mopy import dart_apptest
from mopy import gtest
from mopy.config import Config
from mopy.gn import ConfigForGNArgs, ParseGNConfig
from mopy.log import InitLogging


_logger = logging.getLogger()


def main():
  parser = argparse.ArgumentParser(description="A test runner for gtest "
                                   "application tests.")

  parser.add_argument("--verbose", help="Be verbose (multiple times for more)",
                      default=0, dest="verbose_count", action="count")
  parser.add_argument("apptest_list_file", type=file,
                      help="A file listing apptests to run.")
  parser.add_argument("build_dir", type=str,
                      help="The build output directory.")
  args = parser.parse_args()

  InitLogging(args.verbose_count)
  config = ConfigForGNArgs(ParseGNConfig(args.build_dir))

  execution_globals = {"config": config}
  exec args.apptest_list_file in execution_globals
  apptest_list = execution_globals["tests"]
  _logger.debug("Test list: %s" % apptest_list)

  extra_args = []
  if config.target_os == Config.OS_ANDROID:
    extra_args.extend(android.PrepareShellRun(config, fixed_port=False))

  gtest.set_color()

  exit_code = 0
  for apptest_dict in apptest_list:
    apptest = apptest_dict["test"]
    test_args = apptest_dict.get("test-args", [])
    shell_args = apptest_dict.get("shell-args", []) + extra_args

    _logger.info("Will start: %s" % apptest)
    print "Running %s...." % apptest,
    sys.stdout.flush()

    if apptest_dict.get("type", "gtest") == "dart":
      apptest_result = dart_apptest.run_test(config, apptest_dict, shell_args,
                                             {apptest: test_args})
    else:
      apptest_result = gtest.run_fixtures(config, apptest_dict, apptest,
                                          test_args, shell_args)

    if apptest_result != "Succeeded":
      exit_code = 1
    print apptest_result
    _logger.info("Completed: %s" % apptest)

  return exit_code


if __name__ == '__main__':
  sys.exit(main())
