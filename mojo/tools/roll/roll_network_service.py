#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import sys
import tempfile
import zipfile
from update_from_chromium import chromium_rev_number
from utils import commit
from utils import mojo_root_dir
from utils import system

sys.path.insert(0, os.path.join(mojo_root_dir, "tools"))
# pylint: disable=F0401
import gs

def roll(target_version):
  mojoms_gs_path = "gs://mojo/network/%s/mojoms.zip" % (target_version,)
  network_service_path = os.path.join(mojo_root_dir, "services", "network")
  mojoms_path = os.path.join(network_service_path, "interfaces")
  version_path = os.path.join(network_service_path, "VERSION")

  with tempfile.NamedTemporaryFile() as temp_zip_file:
    gs.download_from_public_bucket(mojoms_gs_path, temp_zip_file.name)
    with zipfile.ZipFile(temp_zip_file.name) as z:
      z.extractall(mojoms_path)

  with open(version_path, 'w') as stamp_file:
    stamp_file.write(target_version)

  system(["git", "add", "interfaces"], cwd=network_service_path)
  system(["git", "add", "VERSION"], cwd=network_service_path)
  chromium_rev = chromium_rev_number(target_version)
  commit("Roll the network service to https://crrev.com/" + chromium_rev,
         cwd=mojo_root_dir)
  return 0

def main():
  parser = argparse.ArgumentParser(
      description="Update the pinned version of the network service " +
                  "and the corresponding checked out mojoms.")
  parser.add_argument("version", help="version to roll to")
  args = parser.parse_args()
  roll(args.version)
  return 0

if __name__ == "__main__":
  sys.exit(main())
