#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script takes linux Go binaries, builds android binaries with NDK tool chain
configured with NDK_PLATFORM and NDK_TOOLCHAIN parameters, zips the stuff and
uploads it to Google Cloud Storage at gs://mojo/go/tool. It also produces
the VERSION file with the sha1 code of the uploaded archive.

This script operates in the INSTALL_DIR directory, so it automatically updates
your current installation of the go binaries on success. On failure it
invalidates your current installation; to fix it, run download.py.

In order to use it, you need:
- depot_tools in your path
- installed android build deps
- WRITE access to GCS

To update go tool binaries you need to
1) run 'gsutil.py config' to update initialize gsutil's credentials
2) run this script: python upload.py path/to/go/binaries.tar.gz
3) push new version of file 'VERSION'

This script doesn't check if current version is already up to date, as the
produced tar.gz archive is slightly different every time since it includes
timestamps.
"""

import hashlib
import os
import shutil
import subprocess
import sys
import tarfile
import tempfile

NDK_PLATFORM = 'android-14'
NDK_TOOLCHAIN = 'arm-linux-androideabi-4.9'

# Path constants. (All of these should be absolute paths.)
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
MOJO_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..'))
# Should be the same as in download.py.
INSTALL_DIR = os.path.join(MOJO_DIR, 'third_party', 'go', 'tool')

sys.path.insert(0, os.path.join(MOJO_DIR, 'tools'))
import find_depot_tools

DEPOT_PATH = find_depot_tools.add_depot_tools_to_path()
GSUTIL_PATH = os.path.join(DEPOT_PATH, 'gsutil.py')

def RunCommand(command, env=None):
  """Run command and return success (True) or failure."""

  print 'Running %s' % (str(command))
  if subprocess.call(command, shell=False, env=env) == 0:
    return True
  print 'Failed.'
  return False

def ExtractBinaries(archive_path):
  """Extracts go binaries from the given tar file to INSTALL_DIR."""

  if os.path.exists(INSTALL_DIR):
    shutil.rmtree(INSTALL_DIR)
  os.mkdir(INSTALL_DIR)
  archive_path = os.path.abspath(archive_path)
  with tarfile.open(archive_path) as arch:
    arch.extractall(INSTALL_DIR)
  os.rename(os.path.join(INSTALL_DIR, 'go'),
            os.path.join(INSTALL_DIR, 'linux_amd64'))

def BuildGoAndroid():
  go_linux = os.path.join(INSTALL_DIR, 'linux_amd64')
  go_android = os.path.join(INSTALL_DIR, 'android_arm')
  # Copy go sources and remove binaries to keep only that we generate.
  shutil.copytree(go_linux, go_android)
  shutil.rmtree(os.path.join(go_android, 'bin'))
  shutil.rmtree(os.path.join(go_android, 'pkg'))
  # Paths required for compilers. These paths can be encoded as a part of the go
  # binaries, so it's better to keep them relative.
  os.chdir(os.path.join(go_android, 'src'))
  third_party = os.path.relpath(os.path.join(MOJO_DIR, 'third_party'))
  ndk_path = os.path.join(third_party, 'android_tools', 'ndk')
  ndk_cc = os.path.join(ndk_path, 'toolchains', NDK_TOOLCHAIN,
      'prebuilt', 'linux-x86_64', 'bin', 'arm-linux-androideabi-gcc')
  sysroot = os.path.join(ndk_path, 'platforms', NDK_PLATFORM, 'arch-arm')
  # Configure environment variables.
  env = os.environ.copy()
  env["GOPATH"] = go_linux
  env["GOROOT"] = go_linux
  env["CC_FOR_TARGET"] = '%s --sysroot %s' % (ndk_cc, sysroot)
  env["GOOS"] = 'android'
  env["GOARCH"] = 'arm'
  env["GOARM"] = '7'
  # Build go tool.
  make_command = ['bash', 'make.bash']
  if not RunCommand(make_command, env=env):
    print "Failed to make go tool for android."
    sys.exit(1)

def Compress():
  """Compresses the go tool into tar.gz and generates sha1 code, renames the
     archive to sha1.tar.gz and returns the sha1 code."""

  print "Compressing go tool, this may take several minutes."
  os.chdir(INSTALL_DIR)
  with tarfile.open(os.path.join('a.tar.gz'), 'w|gz') as arch:
    arch.add('android_arm')
    arch.add('linux_amd64')

  sha1 = ''
  with open(os.path.join(INSTALL_DIR, 'a.tar.gz')) as f:
    sha1 = hashlib.sha1(f.read()).hexdigest()
  os.rename(os.path.join(INSTALL_DIR, 'a.tar.gz'),
            os.path.join(INSTALL_DIR, '%s.tar.gz' % sha1))
  return sha1

def Upload(sha1):
  """Uploads INSTALL_DIR/sha1.tar.gz to Google Cloud Storage under
     gs://mojo/go/tool and writes sha1 to THIS_DIR/VERSION."""

  file_name = '%s.tar.gz' % sha1
  upload_cmd = ['python', GSUTIL_PATH, 'cp',
                '-n', # Do not upload if the file already exists.
                os.path.join(INSTALL_DIR, file_name),
                'gs://mojo/go/tool/%s' % file_name]

  print "Uploading go tool to GCS."
  if not RunCommand(upload_cmd):
    print "Failed to upload go tool to GCS."
    sys.exit(1)
  os.remove(os.path.join(INSTALL_DIR, file_name))
  # Write versions as the last step.
  stamp_file = os.path.join(THIS_DIR, 'VERSION')
  with open(stamp_file, 'w+') as stamp:
    stamp.write('%s\n' % sha1)

  stamp_file = os.path.join(INSTALL_DIR, 'VERSION')
  with open(stamp_file, 'w+') as stamp:
    stamp.write('%s\n' % sha1)

def main():
  ExtractBinaries(sys.argv[1])
  BuildGoAndroid()
  sha1 = Compress()
  Upload(sha1)
  print "Done."

if __name__ == '__main__':
  sys.exit(main())
