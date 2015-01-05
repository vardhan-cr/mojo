#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test runner for gtest application tests."""

import argparse
import ast
import logging
import os
import sys

_logging = logging.getLogger()

from mopy.background_app_group import BackgroundAppGroup
from mopy.config import Config
import mopy.gtest
from mopy.paths import Paths


def main():
  logging.basicConfig()
  # Uncomment to debug:
  #_logging.setLevel(logging.DEBUG)

  parser = argparse.ArgumentParser(description='A test runner for gtest '
                                   'application tests.')

  parser.add_argument('apptest_list_file', type=file,
                      help='A file listing apptests to run.')
  parser.add_argument('build_dir', type=str,
                      help='The build output directory.')
  args = parser.parse_args()

  apptest_list = ast.literal_eval(args.apptest_list_file.read())
  _logging.debug("Test list: %s" % apptest_list)

  mopy.gtest.set_color()
  mojo_paths = Paths(Config(build_dir=args.build_dir))

  exit_code = 0
  for apptest_dict in apptest_list:
    if apptest_dict.get("disabled"):
      continue

    apptest = apptest_dict["test"]
    apptest_args = apptest_dict.get("test-args", [])
    shell_args = apptest_dict.get("shell-args", [])
    launched_services = apptest_dict.get("launched-services", [])

    print "Running " + apptest + "...",
    sys.stdout.flush()

    # List the apptest fixtures so they can be run independently for isolation.
    # TODO(msw): Run some apptests without fixture isolation?
    fixtures = mopy.gtest.get_fixtures(mojo_paths.mojo_shell_path, apptest)

    if not fixtures:
      print "Failed with no tests found."
      exit_code = 1
      continue

    if any(not mojo_paths.IsValidAppUrl(url) for url in launched_services):
      print "Failed with malformed launched-services: %r" % launched_services
      exit_code = 1
      continue

    apptest_result = "Succeeded"
    for fixture in fixtures:
      args_for_apptest = " ".join(["--args-for=" + apptest,
                                   "--gtest_filter=" + fixture] + apptest_args)

      if launched_services:
        success = RunApptestInLauncher(mojo_paths, apptest,
                                       shell_args + [args_for_apptest],
                                       launched_services)
      else:
        success = RunApptestInShell(mojo_paths, apptest,
                                    shell_args + [args_for_apptest])

      if not success:
        apptest_result = "Failed test(s) in %r" % apptest_dict
        exit_code = 1

    print apptest_result

  return exit_code


def RunApptestInShell(mojo_paths, apptest, shell_args):
  return mopy.gtest.run_test([mojo_paths.mojo_shell_path, apptest] + shell_args)


def RunApptestInLauncher(mojo_paths, apptest, shell_args, launched_services):
  with BackgroundAppGroup(mojo_paths, launched_services, shell_args) as apps:
    command = [mojo_paths.mojo_launcher_path,
               '--shell-path=' + apps.socket_path,
               '--app-url=' + apptest,
               '--app-path=' + mojo_paths.FileFromUrl(apptest)]
    return mopy.gtest.run_test(command)


if __name__ == '__main__':
  sys.exit(main())
