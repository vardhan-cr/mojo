#!/usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This script downloads GO_VERSION linux binaries from golang.org, build android
binaries with NDK tool chain configured with NDK_PLATFORM and NDK_TOOLCHAIN
parameters, zips the stuff and uploads it to Google Cloud Storage at
gs://mojo/go/tool. It also produces VERSION file with sha1 code of the uploaded
archive.

In order to use it, you need:
- depot_tools in your path
- installed android build deps
- WRITE access to GCS

To update go tool binaries you need to
1) run 'gsutil.py config' to update initialize gsutil's credentials
2) run this script
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

GO_VERSION = 'go1.4.2'
NDK_PLATFORM = 'android-14'
NDK_TOOLCHAIN = 'arm-linux-androideabi-4.9'

# Path constants. (All of these should be absolute paths.)
THIS_DIR = os.path.abspath(os.path.dirname(__file__))
MOJO_DIR = os.path.abspath(os.path.join(THIS_DIR, '..', '..'))


def RunCommand(command, env=None):
  """Run command and return success (True) or failure."""

  print 'Running %s' % (str(command))
  if subprocess.call(command, shell=False, env=env) == 0:
    return True
  print 'Failed.'
  return False

def DownloadGoLinux(tmpdir):
  """Downloads and go linux_amd64 go binaries from golang.org and extracts them
     to provided tmpdir. The extracted files appear under
     tmpdir/tool/linux_amd64."""

  archive_name = '%s.linux-amd64.tar.gz' % GO_VERSION
  tool_dir = os.path.join(tmpdir, 'tool')
  os.mkdir(tool_dir)
  archive_path = os.path.join(tool_dir, archive_name)
  # Download the Linux x64 Go binaries from golang.org.
  # '-C -': Resume transfer if possible.
  # '--location': Follow Location: redirects.
  # '-o': Output file.
  curl_command = ['curl',
                  '-C', '-',
                  '--location',
                  '-o', archive_path,
                  'http://golang.org/dl/%s' % archive_name]
  if not RunCommand(curl_command):
    print "Failed to linux go binaries from server."
    sys.exit(1)
  with tarfile.open(archive_path) as arch:
    arch.extractall(tool_dir)
  os.remove(archive_path)
  os.rename(os.path.join(tool_dir, 'go'),
            os.path.join(tool_dir, 'linux_amd64'))

def ConfigureAndroidNDK(tmpdir):
  """Creates a standalone tool chain under tmpdir. Returns path to ndk cc
     compiler."""

  ndk_root = os.path.join(tmpdir, 'ndk-toolchain')
  os.chdir(os.path.join(MOJO_DIR, 'third_party', 'android_tools', 'ndk'))
  script = os.path.join('build', 'tools', 'make-standalone-toolchain.sh')
  make_command = ['bash', script,
                  '--platform=%s' % NDK_PLATFORM,
                  '--toolchain=%s' % NDK_TOOLCHAIN,
                  '--install-dir=%s' % ndk_root]
  if not RunCommand(make_command):
    print "Failed to generate NDK tool chain."
    sys.exit(1)
  return os.path.join(ndk_root, 'bin', 'arm-linux-androideabi-gcc')

def BuildGoAndroid(tmpdir):
  ndk_cc = ConfigureAndroidNDK(tmpdir)
  go_linux = os.path.join(tmpdir, 'tool', 'linux_amd64')
  go_android = os.path.join(tmpdir, 'tool', 'android_arm')
  # Copy go sources and remove binaries to keep only that we generate.
  shutil.copytree(go_linux, go_android)
  shutil.rmtree(os.path.join(go_android, 'bin'))
  shutil.rmtree(os.path.join(go_android, 'pkg'))
  # Configure environment variables.
  env = os.environ.copy()
  env["GOPATH"] = go_linux
  env["GOROOT"] = go_linux
  env["CC_FOR_TARGET"] = ndk_cc
  env["GOOS"] = 'android'
  env["GOARCH"] = 'arm'
  env["GOARM"] = '7'
  # Build go tool.
  os.chdir(os.path.join(go_android, 'src'))
  make_command = ['bash', 'make.bash']
  if not RunCommand(make_command, env=env):
    print "Failed to make go tool for android."
    sys.exit(1)

def Compress(tmpdir):
  """Compresses the go tool into tar.gz and generates sha1 code, renames the
     archive to sha1.tar.gz and returns the sha1 code."""

  print "Compressing go tool, this may take several minutes."
  os.chdir(os.path.join(tmpdir, 'tool'))
  with tarfile.open(os.path.join('..', 'a.tar.gz'), 'w|gz') as arch:
    arch.add('android_arm')
    arch.add('linux_amd64')

  sha1 = ''
  with open(os.path.join(tmpdir, 'a.tar.gz')) as f:
    sha1 = hashlib.sha1(f.read()).hexdigest()
  os.rename(os.path.join(tmpdir, 'a.tar.gz'),
            os.path.join(tmpdir, '%s.tar.gz' % sha1))
  return sha1

def Upload(tmpdir, sha1):
  """Uploads tmpdir/sha1.tar.gz to Google Cloud Storage under gs://mojo/go/tool
     and writes sha1 to THIS_DIR/VERSION."""

  stamp_file = os.path.join(THIS_DIR, 'VERSION')
  with open(stamp_file, 'w+') as stamp:
    stamp.write('%s\n' % sha1)
  file_name = '%s.tar.gz' % sha1
  upload_cmd = ['gsutil.py', 'cp',
                '-n', # Do not upload if the file already exists.
                os.path.join(tmpdir, file_name),
                'gs://mojo/go/tool/%s' % file_name]

  print "Uploading go tool to GCS."
  if not RunCommand(upload_cmd):
    print "Failed to upload go tool to GCS."
    sys.exit(1)

def main():
  tmpdir = tempfile.mkdtemp(prefix='mojo')
  print "Created temp dir %s" % tmpdir
  DownloadGoLinux(tmpdir)
  BuildGoAndroid(tmpdir)
  sha1 = Compress(tmpdir)
  Upload(tmpdir, sha1)
  shutil.rmtree(tmpdir)
  print "Done."

if __name__ == '__main__':
  sys.exit(main())
