#!/usr/bin/env python
#
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility for updating third_party packages used in the Mojo Dart SDK"""

import argparse
import errno
import json
import os
import shutil
import subprocess
import sys

SCRIPT_DIR = os.path.dirname(sys.argv[0])

def check_for_pubspec_yaml():
  return os.path.exists(os.path.join(SCRIPT_DIR, 'pubspec.yaml'))

def change_directory(directory):
  return os.chdir(directory)

def remove_existing_packages(base_path):
  print('Removing all package directories under %s' % base_path)
  for child in os.listdir(base_path):
    path = os.path.join(base_path, child)
    if os.path.isdir(path):
      print('Removing %s ' % path)
      shutil.rmtree(path)

def pub_get(pub_exe):
  return subprocess.check_output([pub_exe, 'get'])

def copy_packages(base_path):
  packages_path = os.path.join(base_path, 'packages')
  for package in os.listdir(packages_path):
    lib_path = os.path.realpath(os.path.join(packages_path, package))
    package_path = os.path.normpath(os.path.join(lib_path, '..'))
    destinaton_path = os.path.join(base_path, package)
    print('Copying %s to %s' % (package_path, destinaton_path))
    shutil.copytree(package_path, destinaton_path)

def cleanup(base_path):
  shutil.rmtree(os.path.join(base_path, 'packages'), True)
  shutil.rmtree(os.path.join(base_path, '.pub'), True)
  os.remove(os.path.join(base_path, '.packages'))

def main():
  parser = argparse.ArgumentParser(description='Update third_party packages')
  parser.add_argument('--pub-exe',
                      action='store',
                      metavar='pub_exe',
                      help='Path to the pub executable',
                      default='../../../../third_party/dart-sdk/dart-sdk/bin/pub')

  args = parser.parse_args()
  if not check_for_pubspec_yaml():
    print('Error could not find pubspec.yaml')
    return 1

  remove_existing_packages(SCRIPT_DIR)
  change_directory(SCRIPT_DIR)
  print('Running pub get')
  pub_get(args.pub_exe)
  copy_packages(SCRIPT_DIR)
  print('Cleaning up')
  cleanup(SCRIPT_DIR)

if __name__ == '__main__':
  sys.exit(main())
