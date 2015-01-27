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

from mopy import android
from mopy import gtest
from mopy.background_app_group import BackgroundAppGroup
from mopy.config import Config
from mopy.gn import ConfigForGNArgs, ParseGNConfig
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

  config = ConfigForGNArgs(ParseGNConfig(args.build_dir))
  is_android = (config.target_os == Config.OS_ANDROID)
  android_context = android.PrepareShellRun(config) if is_android else None

  gtest.set_color()
  mojo_paths = Paths(config)

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
    fixtures = gtest.get_fixtures(config, apptest, android_context)

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
      args_for_apptest = apptest_args + ["--gtest_filter=%s" % fixture]
      if launched_services:
        success = RunApptestInLauncher(config, mojo_paths, apptest,
                                       args_for_apptest, shell_args,
                                       launched_services)
      else:
        success = RunApptestInShell(config, apptest, args_for_apptest,
                                    shell_args, android_context)

      if not success:
        apptest_result = "Failed test(s) in %r" % apptest_dict
        exit_code = 1

    print apptest_result

  return exit_code


def RunApptestInShell(config, application, application_args, shell_args,
                      android_context):
  return gtest.run_test(config, shell_args,
                        {application: application_args}, android_context)


def RunApptestInLauncher(config, mojo_paths, application, application_args,
                         shell_args, launched_services):
  with BackgroundAppGroup(
      mojo_paths, launched_services,
      gtest.build_shell_arguments(shell_args,
                                  {application: application_args},
                                  run_apps=False)) as apps:
    launcher_args = [
        '--shell-path=' + apps.socket_path,
        '--app-url=' + application,
        '--app-path=' + mojo_paths.FileFromUrl(application)]
    return gtest.run_test(config, launcher_args, run_launcher=True)


if __name__ == '__main__':
  sys.exit(main())
