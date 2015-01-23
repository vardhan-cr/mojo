#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import sys
import tempfile
import zipfile

_PLATFORMS = ["linux-x64", "android-arm"]

if not sys.platform.startswith("linux"):
  print "Not supported for your platform"
  sys.exit(0)

# Add //tools to the system path so that we can import the gs helper.
script_dir = os.path.dirname(os.path.realpath(__file__))
tools_path = os.path.join(script_dir, os.pardir, os.pardir, "tools")
sys.path.insert(0, tools_path)
# pylint: disable=F0401
import gs


def download_binary(version, platform, prebuilt_directory):
  gs_path = "gs://mojo/network/%s/%s.zip" % (version, platform)
  output_directory = os.path.join(prebuilt_directory, platform)
  binary_name = "network_service.mojo"

  with tempfile.NamedTemporaryFile() as temp_zip_file:
    gs.download_from_public_bucket(gs_path, temp_zip_file.name)
    with zipfile.ZipFile(temp_zip_file.name) as z:
      zi = z.getinfo(binary_name)
      mode = zi.external_attr >> 16
      z.extract(zi, output_directory)
      os.chmod(os.path.join(output_directory, binary_name), mode)


def main():
  parser = argparse.ArgumentParser(
      description="Download prebuilt network service binaries from google " +
                  "storage")
  parser.parse_args()

  prebuilt_directory_path = os.path.join(script_dir, "prebuilt")
  stamp_path = os.path.join(prebuilt_directory_path, "STAMP")
  version_path = os.path.join(script_dir, "VERSION")
  with open(version_path) as version_file:
    version = version_file.read().strip()

  try:
    with open(stamp_path) as stamp_file:
      current_version = stamp_file.read().strip()
      if current_version == version:
        return 0  # Already have the right version.
  except IOError:
    pass  # If the stamp file does not exist we need to download a new binary.

  for platform in _PLATFORMS:
    download_binary(version, platform, prebuilt_directory_path)

  with open(stamp_path, 'w') as stamp_file:
    stamp_file.write(version)
  return 0


if __name__ == "__main__":
  sys.exit(main())
