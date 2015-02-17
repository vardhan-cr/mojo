#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Pulls down the current dart sdk to third_party/dart-sdk/."""

import os
import subprocess
import sys

# Path constants. (All of these should be absolute paths.)
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
CHROMIUM_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..'))
DART_SDK_DIR = os.path.join(CHROMIUM_DIR, 'third_party', 'dart-sdk')

# TODO(erg): We might want 32 bit linux too? I don't know of anyone who still
# uses that though. It looks like clang isn't built 32 bit.

LINUX_64_SDK = ('http://gsdview.appspot.com/dart-archive/channels/dev/' +
                'raw/43808/sdk/dartsdk-linux-x64-release.zip')

OUTPUT_FILE = os.path.join(DART_SDK_DIR, 'dartsdk-linux-x64-release.zip')

def RunCommand(command, fail_hard=True):
  """Run command and return success (True) or failure; or if fail_hard is
     True, exit on failure."""

  print 'Running %s' % (str(command))
  if subprocess.call(command, shell=False) == 0:
    return True
  print 'Failed.'
  if fail_hard:
    sys.exit(1)
  return False

def main():
  # For version one, we don't actually redownload if the sdk is already
  # present. This will be replaced with download_from_google_storage once
  # it supports tarballs.
  #
  # You can explicitly redownload this by blowing away your
  # third_party/dart-sdk/ directory.
  if not os.path.exists(OUTPUT_FILE):
    wget_command = ['wget', '-N', '-c', LINUX_64_SDK, '-P', DART_SDK_DIR]
    if not RunCommand(wget_command, fail_hard=False):
      print "Failed to get dart sdk from server."
      return

    unzip_command = ['unzip', '-q', OUTPUT_FILE, '-d', DART_SDK_DIR]
    if not RunCommand(unzip_command, fail_hard=False):
      print "Failed to unzip the dart sdk."

if __name__ == '__main__':
  sys.exit(main())
