#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A "smart" test runner for gtest unit tests (that caches successes)."""

import argparse
import ast
import logging
import os
import platform
import subprocess
import sys

_logging = logging.getLogger()

import mopy.gtest
from mopy.config import Config
from mopy.gn import ConfigForGNArgs, ParseGNConfig
from mopy.paths import Paths
from mopy.transitive_hash import file_hash, transitive_hash

paths = Paths()

def main():
  logging.basicConfig()
  # Uncomment to debug:
  # _logging.setLevel(logging.DEBUG)

  parser = argparse.ArgumentParser(
      description="A 'smart' test runner for gtest unit tests (that caches "
                  "successes).")

  parser.add_argument("gtest_list_file",
                      help="The file containing the tests to run.",
                      type=file)
  parser.add_argument("root_dir", help="The build directory.")
  parser.add_argument("successes_cache_filename",
                      help="The file caching test results.", default=None,
                      nargs='?')
  args = parser.parse_args()

  _logging.debug("Test list file: %s", args.gtest_list_file)
  gtest_list = ast.literal_eval(args.gtest_list_file.read())
  _logging.debug("Test list: %s" % gtest_list)

  config = ConfigForGNArgs(ParseGNConfig(args.root_dir))

  print "Running tests in directory: %s" % args.root_dir
  os.chdir(args.root_dir)

  if args.successes_cache_filename:
    print "Successes cache file: %s" % args.successes_cache_filename
  else:
    print "No successes cache file (will run all tests unconditionally)"

  if args.successes_cache_filename:
    # This file simply contains a list of transitive hashes of tests that
    # succeeded.
    try:
      _logging.debug("Trying to read successes cache file: %s",
                     args.successes_cache_filename)
      with open(args.successes_cache_filename, 'rb') as f:
        successes = set([x.strip() for x in f.readlines()])
      _logging.debug("Successes: %s", successes)
    except IOError:
      # Just assume that it didn't exist, or whatever.
      print ("Failed to read successes cache file %s (will create)" %
             args.successes_cache_filename)
      successes = set()

  mopy.gtest.set_color()

  # TODO(vtl): We may not close this file on failure.
  successes_cache_file = open(args.successes_cache_filename, 'ab') \
      if args.successes_cache_filename else None
  for gtest_dict in gtest_list:
    if gtest_dict.get("disabled"):
      continue
    if not config.match_target_os(gtest_dict.get("target_os", [])):
      continue

    gtest = gtest_dict["test"]
    cacheable = gtest_dict.get("cacheable", True)
    if not cacheable:
      _logging.debug("%s is marked as non-cacheable" % gtest)

    gtest_file = gtest
    if platform.system() == 'Windows':
      gtest_file += ".exe"
    if config.target_os == Config.OS_ANDROID:
      gtest_file = gtest + "_apk/" + gtest + "-debug.apk"

    if successes_cache_file and cacheable:
      _logging.debug("Getting transitive hash for %s ... " % gtest)
      try:
        if config.target_os == Config.OS_ANDROID:
          gtest_hash = file_hash(gtest_file)
        else:
          gtest_hash = transitive_hash(gtest_file)
      except subprocess.CalledProcessError:
        print "Failed to get transitive hash for %s" % gtest
        return 1
      _logging.debug("  Transitive hash: %s" % gtest_hash)

      if gtest_hash in successes:
        print "Skipping %s (previously succeeded)" % gtest
        continue

    print "Running %s...." % gtest,
    sys.stdout.flush()
    try:
      if config.target_os == Config.OS_ANDROID:
        command = [
            "python",
            os.path.join(paths.src_root, "build", "android", "test_runner.py"),
            "gtest",
            "--output-directory",
            args.root_dir,
            "-s",
            gtest,
        ]
      else:
        command = ["./" + gtest]
      subprocess.check_output(command, stderr=subprocess.STDOUT)
      print "Succeeded"
      # Record success.
      if args.successes_cache_filename and cacheable:
        successes.add(gtest_hash)
        successes_cache_file.write(gtest_hash + '\n')
        successes_cache_file.flush()
    except subprocess.CalledProcessError as e:
      print "Failed with exit code %d and output:" % e.returncode
      print 72 * '-'
      print e.output
      print 72 * '-'
      return 1
    except OSError as e:
      print "  Failed to start test"
      return 1
  print "All tests succeeded"
  if successes_cache_file:
    successes_cache_file.close()

  return 0

if __name__ == '__main__':
  sys.exit(main())
