#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Pulls down the current dart sdk to third_party/dart-sdk/.

You can manually force this to run again by removing
third_party/dart-sdk/STAMP_FILE, which contains the URL of the SDK that
was downloaded. Rolling works by updating LINUX_64_SDK to a new URL.
"""

import os
import shutil
import subprocess
import sys

# How to roll the dart sdk: Just change this url! We write this to the stamp
# file after we download, and then check the stamp file for differences.
LINUX_64_SDK = ('http://gsdview.appspot.com/dart-archive/channels/dev/' +
                'raw/44260/sdk/dartsdk-linux-x64-release.zip')

# Path constants. (All of these should be absolute paths.)
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
MOJO_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..'))
DART_SDK_DIR = os.path.join(MOJO_DIR, 'third_party', 'dart-sdk')
OUTPUT_FILE = os.path.join(DART_SDK_DIR, 'dartsdk-linux-x64-release.zip')
STAMP_FILE = os.path.join(DART_SDK_DIR, 'STAMP_FILE')
LIBRARIES_FILE = os.path.join(DART_SDK_DIR,'dart-sdk',
                              'lib', '_internal', 'libraries.dart')
PATCH_FILE = os.path.join(MOJO_DIR, 'tools', 'dart', 'patch_sdk.diff')

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
  # Only get the SDK if we don't have a stamp for or have an out of date stamp
  # file.
  get_sdk = False
  if not os.path.exists(STAMP_FILE):
    get_sdk = True
  else:
    # Get the contents of the stamp file.
    with open(STAMP_FILE, "r") as stamp_file:
      stamp_url = stamp_file.read().replace('\n', '')
      if stamp_url != LINUX_64_SDK:
        get_sdk = True

  if get_sdk:
    # Completely remove all traces of the previous SDK.
    if os.path.exists(DART_SDK_DIR):
      shutil.rmtree(DART_SDK_DIR)
    os.mkdir(DART_SDK_DIR)

    # Download the Linux x64 based Dart SDK.
    # '-C -': Resume transfer if possible.
    # '--location': Follow Location: redirects.
    # '-o': Output file.
    curl_command = ['curl',
                    '-C', '-',
                    '--location',
                    '-o', OUTPUT_FILE,
                    LINUX_64_SDK]
    if not RunCommand(curl_command, fail_hard=False):
      print "Failed to get dart sdk from server."
      return

    # Write our stamp file so we don't redownload the sdk.
    with open(STAMP_FILE, "w") as stamp_file:
      stamp_file.write(LINUX_64_SDK)

  unzip_command = ['unzip', '-o', '-q', OUTPUT_FILE, '-d', DART_SDK_DIR]
  if not RunCommand(unzip_command, fail_hard=False):
    print "Failed to unzip the dart sdk."
    return

  # Patch the the dart-sdk/lib/_internal/libraries.dart file
  # so that it understands dart:sky imports.
  patch_command = ['patch', LIBRARIES_FILE, PATCH_FILE]
  if not RunCommand(patch_command, fail_hard=False):
    print "Failed to apply the patch to the Dart libraries file."
    return

if __name__ == '__main__':
  sys.exit(main())
