#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import subprocess
import sys
import tempfile
import urllib2
import zipfile

from update_from_chromium import chromium_rev_number
from utils import commit
from utils import mojo_root_dir
from utils import system

sys.path.insert(0, os.path.join(mojo_root_dir, "tools"))
# pylint: disable=F0401
import gs

def roll(target_version):
  try:
    chromium_rev = chromium_rev_number(target_version)
  except urllib2.HTTPError:
    print ("Failed to identify a Chromium revision associated with %s. "
           "Ensure that target_version is a Chromium origin/master "
           "commit.") % (target_version)
    return 1

  mojoms_gs_path = "gs://mojo/network_service/%s/mojoms.zip" % (target_version,)
  network_service_path = os.path.join(
      mojo_root_dir, "mojo", "services", "network")
  mojoms_path = os.path.join(network_service_path, "public", "interfaces")
  mojo_public_tools_path = os.path.join(
      mojo_root_dir, "mojo", "public", "tools")
  version_path = os.path.join(mojo_public_tools_path, "NETWORK_SERVICE_VERSION")

  try:
    with tempfile.NamedTemporaryFile() as temp_zip_file:
      gs.download_from_public_bucket(mojoms_gs_path, temp_zip_file.name)

      try:
        system(["git", "rm", "-r", mojoms_path], cwd=mojo_root_dir)
      except subprocess.CalledProcessError:
        print ("Could not remove %s. "
               "Ensure your local tree is in a clean state." % mojoms_path)
        return 1

      with zipfile.ZipFile(temp_zip_file.name) as z:
        z.extractall(mojoms_path)
  # pylint: disable=C0302,bare-except
  except:
    print ("Failed to download the mojom files associated with %s. Ensure that "
           "the corresponding network service artifacts were uploaded to "
           "Google Storage.") % (target_version)
    return 1

  with open(version_path, 'w') as stamp_file:
    stamp_file.write(target_version)

  system(["git", "add", "public"], cwd=network_service_path)
  system(["git", "add", "NETWORK_SERVICE_VERSION"], cwd=mojo_public_tools_path)
  commit("Roll the network service to https://crrev.com/" + chromium_rev,
         cwd=mojo_root_dir)
  return 0

def main():
  parser = argparse.ArgumentParser(
      description="Update the pinned version of the network service " +
                  "and the corresponding checked out mojoms.")
  parser.add_argument("version", help="version to roll to (a Chromium "
                                      "origin/master commit)")
  args = parser.parse_args()
  roll(args.version)
  return 0

if __name__ == "__main__":
  sys.exit(main())
