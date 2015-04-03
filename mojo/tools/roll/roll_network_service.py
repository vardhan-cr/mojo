#!/usr/bin/env python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import subprocess
import sys
import tempfile
import urllib2
import zipfile

from update_from_chromium import chromium_rev_number
from utils import commit
from utils import mojo_root_dir
from utils import system

import patch

sys.path.insert(0, os.path.join(mojo_root_dir, "mojo/public/tools/pylib"))
# pylint: disable=F0401
import gs

def roll(target_version, custom_build):
  find_depot_tools_path = os.path.join(mojo_root_dir, "tools")
  sys.path.insert(0, find_depot_tools_path)
  # pylint: disable=F0401
  import find_depot_tools
  depot_tools_path = find_depot_tools.add_depot_tools_to_path()

  if custom_build:
    match = re.search(
        "^custom_build_base_([^_]+)_issue_([0-9]+)_patchset_([0-9]+)$",
        target_version)
    if not match:
      print "Failed to parse the version name."
      return 1
    chromium_commit_hash = match.group(1)
    rietveld_issue = match.group(2)
    rietveld_patchset = match.group(3)
  else:
    chromium_commit_hash = target_version

  try:
    chromium_rev = chromium_rev_number(chromium_commit_hash)
  except urllib2.HTTPError:
    print ("Failed to identify a Chromium revision associated with %s. "
           "Ensure that it is a Chromium origin/master "
           "commit.") % (chromium_commit_hash)
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
      gs.download_from_public_bucket(mojoms_gs_path, temp_zip_file.name,
                                     depot_tools_path)

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

  if custom_build:
    commit_message = ("Roll the network service to a custom build created from "
                      "https://crrev.com/%s/#ps%s") % (rietveld_issue,
                                                       rietveld_patchset)
  else:
    commit_message = ("Roll the network service to "
                      "https://crrev.com/%s") % chromium_rev
  commit(commit_message, cwd=mojo_root_dir)
  return 0

def main():
  parser = argparse.ArgumentParser(
      description="Update the pinned version of the network service " +
                  "and the corresponding checked out mojoms.")
  parser.add_argument(
      "--custom-build", action="store_true",
      help="Indicates that this is a build with change that is not committed. "
           "The change must be uploaded to Rietveld.")
  parser.add_argument(
      "version",
      help="Version to roll to. If --custom-build is not specified, this "
           "should be a Chromium origin/master commit; otherwise, this should "
           "be in the format of custom_build_base_<base_commit>_"
           "issue_<rietveld_issue>_patchset_<rietveld_patchset>.")
  args = parser.parse_args()

  roll(args.version, args.custom_build)

  try:
    patch.patch("network_service_patches")
  except subprocess.CalledProcessError:
    print "ERROR: Roll failed due to a patch not applying"
    print "Fix the patch to apply, commit the result, and re-run this script"
    return 1

  return 0

if __name__ == "__main__":
  sys.exit(main())
