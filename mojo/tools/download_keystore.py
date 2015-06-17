#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tool to download keys needed to sign the official Mojo Shell APK build.
"""

import argparse
import os
import subprocess
import sys

import mopy.gn as gn
from mopy.config import Config
from mopy.paths import Paths


# Google Storage path of the official keystore to download.
_KEYSTORE_PATH = "gs://mojo/android/keys/mojo_shell-official.keystore"


def _download(config, source, dest, dry_run):
  paths = Paths(config)
  sys.path.insert(0, os.path.join(paths.src_root, "tools"))
  # pylint: disable=F0401
  import find_depot_tools

  depot_tools_path = find_depot_tools.add_depot_tools_to_path()
  gsutil_exe = os.path.join(depot_tools_path, "third_party", "gsutil", "gsutil")

  if dry_run:
    print str([gsutil_exe, "cp", source, dest])
  else:
    subprocess.check_call([gsutil_exe, "cp", source, dest])


def _download_keys(config, directory_path, dry_run, verbose):
  if verbose:
    print "Downloading " + _KEYSTORE_PATH + " to " + directory_path
  _download(config, _KEYSTORE_PATH, directory_path, dry_run)


def main():
  parser = argparse.ArgumentParser(
      description="Downloads the keystore used to sign official APK builds.")
  parser.add_argument("-n", "--dry_run", help="Dry run, do not actually "+
      "download", action="store_true")
  parser.add_argument("-v", "--verbose", help="Verbose mode",
      action="store_true")
  parser.add_argument("output", nargs=1, type=str,
                      help="Output path to save the keystore to")
  args = parser.parse_args()

  target_os = Config.OS_ANDROID
  config = Config(target_os=target_os, is_debug=False, is_official_build=True)
  _download_keys(config, args.output[0], args.dry_run, args.verbose)

  return 0


if __name__ == "__main__":
  sys.exit(main())
