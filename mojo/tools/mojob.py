#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This a simple script to make building/testing Mojo components easier.

import argparse
from copy import deepcopy
import os
import platform
import re
import subprocess
import sys

from get_test_list import GetTestList
from mopy.config import Config
from mopy.paths import Paths


def _args_to_config(args):
  # Default to host OS.
  target_os = None
  if args.android:
    target_os = Config.OS_ANDROID
  elif args.chromeos:
    target_os = Config.OS_CHROMEOS

  if args.cpu_arch is None and args.android:
    args.cpu_arch = 'arm'
  target_arch = args.cpu_arch

  additional_args = {}

  if 'clang' in args:
    additional_args['is_clang'] = args.clang

  if 'asan' in args and args.asan:
    additional_args['sanitizer'] = Config.SANITIZER_ASAN

  # Additional non-standard config entries:

  if 'goma' in args:
    goma_dir = os.environ.get('GOMA_DIR')
    goma_home_dir = os.path.join(os.getenv('HOME', ''), 'goma')
    if args.goma and goma_dir:
      additional_args['use_goma'] = True
      additional_args['goma_dir'] = goma_dir
    elif args.goma and os.path.exists(goma_home_dir):
      additional_args['use_goma'] = True
      additional_args['goma_dir'] = goma_home_dir
    else:
      additional_args['use_goma'] = False
      additional_args['goma_dir'] = None

  if 'with_dart' in args:
    additional_args['with_dart'] = args.with_dart

  additional_args['use_nacl'] = args.nacl

  if 'dry_run' in args:
    additional_args['dry_run'] = args.dry_run

  if 'builder_name' in args:
    additional_args['builder_name'] = args.builder_name
  if 'build_number' in args:
    additional_args['build_number'] = args.build_number
  if 'master_name' in args:
    additional_args['master_name'] = args.master_name
  if 'test_results_server' in args:
    additional_args['test_results_server'] = args.test_results_server

  return Config(target_os=target_os, target_arch=target_arch,
                is_debug=args.debug, **additional_args)


def _get_out_dir(config):
  paths = Paths(config)
  return paths.SrcRelPath(paths.build_dir)


def sync(config):
  # pylint: disable=W0613
  return subprocess.call(['gclient', 'sync'])


def gn(config):
  command = ['gn', 'gen', '--check']

  gn_args = []
  gn_args.append('is_debug=' + ('true' if config.is_debug else 'false'))
  gn_args.append('is_asan=' + ('true' if config.sanitizer ==
                               Config.SANITIZER_ASAN else 'false'))
  if config.is_clang is not None:
    gn_args.append('is_clang=' + ('true' if config.is_clang else 'false'))
  else:
    gn_args.append('is_clang=' + ('true' if config.target_os not in
                                  (Config.OS_ANDROID, Config.OS_WINDOWS)
                                  else 'false'))

  if config.values['use_goma']:
    gn_args.append('use_goma=true')
    gn_args.append(r'''goma_dir=\"%s\"''' % config.values['goma_dir'])
  else:
    gn_args.append('use_goma=false')

  if config.values['with_dart']:
    gn_args.append('mojo_use_dart=true')

  if config.values['use_nacl']:
    gn_args.append('mojo_use_nacl=true')

  if config.target_os == Config.OS_ANDROID:
    gn_args.append(r'''os=\"android\"''')
  elif config.target_os == Config.OS_CHROMEOS:
    gn_args.append(r'''os=\"chromeos\" use_system_harfbuzz=false''')

  gn_args.append(r'''cpu_arch=\"%s\"''' % config.target_arch)

  out_dir = _get_out_dir(config)
  command.append(out_dir)
  command.append('--args="%s"' % ' '.join(gn_args))

  print 'Running %s ...' % ' '.join(command)
  return subprocess.call(' '.join(command), shell=True)


def _get_gn_arg_value(out_dir, arg):
  args_file_path = os.path.join(out_dir, 'args.gn')
  if os.path.isfile(args_file_path):
    key_value_regex = re.compile(r'^%s = (.+)$' % arg)
    with open(args_file_path, 'r') as args_file:
      for line in args_file.readlines():
        m = key_value_regex.search(line)
        if m:
          return m.group(1).strip('"')
  return ''


def build(config):
  out_dir = _get_out_dir(config)
  print 'Building in %s ...' % out_dir
  if _get_gn_arg_value(out_dir, 'use_goma') == 'true':
    # Use the configured goma directory.
    local_goma_dir = _get_gn_arg_value(out_dir, 'goma_dir')
    print 'Ensuring goma (in %s) started ...' % local_goma_dir
    command = ['python',
               os.path.join(local_goma_dir, 'goma_ctl.py'),
               'ensure_start']
    exit_code = subprocess.call(command)
    if exit_code:
      return exit_code

    return subprocess.call(['ninja', '-j', '1000', '-l', '100', '-C', out_dir])
  else:
    return subprocess.call(['ninja', '-C', out_dir])


def _run_tests(config, test_types):
  """Runs the tests of the given type(s) for the given config."""

  assert isinstance(test_types, list)
  config = deepcopy(config)
  config.values['test_types'] = test_types

  test_list = GetTestList(config)
  dry_run = config.values.get('dry_run')
  final_exit_code = 0
  for entry in test_list:
    print 'Running: %s' % entry['name']
    print 'Command: %s' % entry['command']
    if dry_run:
      continue

    exit_code = subprocess.call(entry['command'])
    if not final_exit_code:
      final_exit_code = exit_code
  return final_exit_code


def test(config):
  return _run_tests(config, [Config.TEST_TYPE_DEFAULT])


def perftest(config):
  return _run_tests(config, [Config.TEST_TYPE_PERF])


def pytest(config):
  return _run_tests(config, ['python'])


def darttest(config):
  return _run_tests(config, ['dart'])


def nacltest(config):
  return _run_tests(config, ['nacl'])


def main():
  os.chdir(Paths().src_root)

  parser = argparse.ArgumentParser(description='A script to make building'
      '/testing Mojo components easier.')

  parent_parser = argparse.ArgumentParser(add_help=False)
  parent_parser.add_argument('--asan', help='Use Address Sanitizer',
                             action='store_true')

  debug_group = parent_parser.add_mutually_exclusive_group()
  debug_group.add_argument('--debug', help='Debug build (default)',
                           default=True, action='store_true')
  debug_group.add_argument('--release', help='Release build', default=False,
                           dest='debug', action='store_false')

  os_group = parent_parser.add_mutually_exclusive_group()
  os_group.add_argument('--android', help='Build for Android',
                        action='store_true')
  os_group.add_argument('--chromeos', help='Build for ChromeOS',
                        action='store_true')

  parent_parser.add_argument('--cpu-arch',
                             help='CPU architecture to build for.',
                             choices=['x64', 'x86', 'arm'])

  parent_parser.add_argument('--nacl', help='Add in NaCl', default=False,
                             action='store_true')

  subparsers = parser.add_subparsers()

  sync_parser = subparsers.add_parser('sync', parents=[parent_parser],
      help='Sync using gclient (does not run gn).')
  sync_parser.set_defaults(func=sync)

  gn_parser = subparsers.add_parser('gn', parents=[parent_parser],
                                    help='Run gn for mojo (does not sync).')
  gn_parser.set_defaults(func=gn)
  gn_parser.add_argument('--with-dart', help='Configure the Dart bindings',
                         action='store_true')
  clang_group = gn_parser.add_mutually_exclusive_group()
  clang_group.add_argument('--clang', help='Use Clang (default)', default=None,
                           action='store_true')
  clang_group.add_argument('--gcc', help='Use GCC',
                           dest='clang', action='store_false')
  goma_group = gn_parser.add_mutually_exclusive_group()
  goma_group.add_argument('--goma',
                          help='Use Goma (if $GOMA_DIR is set or $HOME/goma '
                               'exists; default)',
                          default=True,
                          action='store_true')
  goma_group.add_argument('--no-goma', help='Don\'t use Goma', default=False,
                          dest='goma', action='store_false')

  build_parser = subparsers.add_parser('build', parents=[parent_parser],
                                       help='Build')
  build_parser.set_defaults(func=build)

  test_parser = subparsers.add_parser('test', parents=[parent_parser],
                                      help='Run unit tests (does not build).')
  test_parser.set_defaults(func=test)
  test_parser.add_argument('--dry-run',
                           help='Print instead of executing commands',
                           default=False, action='store_true')

  perftest_parser = subparsers.add_parser('perftest', parents=[parent_parser],
      help='Run perf tests (does not build).')
  perftest_parser.set_defaults(func=perftest)

  pytest_parser = subparsers.add_parser('pytest', parents=[parent_parser],
      help='Run Python unit tests (does not build).')
  pytest_parser.set_defaults(func=pytest)

  darttest_parser = subparsers.add_parser('darttest', parents=[parent_parser],
      help='Run Dart unit tests (does not build).')
  darttest_parser.set_defaults(func=darttest)

  nacltest_parser = subparsers.add_parser('nacltest', parents=[parent_parser],
      help='Run NaCl unit tests (does not build).')
  nacltest_parser.set_defaults(func=nacltest)

  args = parser.parse_args()
  config = _args_to_config(args)
  return args.func(config)


if __name__ == '__main__':
  sys.exit(main())
