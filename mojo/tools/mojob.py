#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This a simple script to make building/testing Mojo components easier.

import argparse
from copy import deepcopy
import os
import subprocess
import sys

from get_test_list import GetTestList
from mopy.config import Config
from mopy.paths import Paths
from mopy.gn import GNArgsForConfig, ParseGNConfig, CommandLineForGNArgs


def _args_to_config(args):
  # Default to host OS.
  target_os = None
  if args.android:
    target_os = Config.OS_ANDROID
  elif args.chromeos:
    target_os = Config.OS_CHROMEOS

  target_cpu = args.target_cpu

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

  return Config(target_os=target_os, target_cpu=target_cpu,
                is_debug=args.debug, dcheck_always_on=args.dcheck_always_on,
                **additional_args)


def _get_out_dir(config):
  paths = Paths(config)
  return paths.SrcRelPath(paths.build_dir)


def sync(config):
  # pylint: disable=W0613
  return subprocess.call(['gclient', 'sync'])


def gn(config):
  command = ['gn', 'gen', '--check']

  gn_args = CommandLineForGNArgs(GNArgsForConfig(config))
  out_dir = _get_out_dir(config)
  command.append(out_dir)
  command.append('--args=%s' % ' '.join(gn_args))

  print 'Running %s %s ...' % (command[0],
                               ' '.join('\'%s\'' % x for x in command[1:]))
  return subprocess.call(command)


def build(config):
  out_dir = _get_out_dir(config)
  gn_args = ParseGNConfig(out_dir)
  print 'Building in %s ...' % out_dir
  if gn_args.get('use_goma'):
    # Use the configured goma directory.
    local_goma_dir = gn_args.get('goma_dir')
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


def dartcheck(config):
  """Runs the dart analyzer on code for the given config."""

  out_dir = _get_out_dir(config)
  print 'Checking dart code in %s ...' % out_dir
  return subprocess.call(['ninja', '-C', out_dir, 'dartcheck'])


def _run_tests(config, test_types):
  """Runs the tests of the given type(s) for the given config."""

  assert isinstance(test_types, list)
  config = deepcopy(config)
  config.values['test_types'] = test_types

  test_list = GetTestList(config)
  dry_run = config.values.get('dry_run')
  final_exit_code = 0
  failure_list = []
  for entry in test_list:
    print 'Running: %s' % entry['name']
    print 'Command: %s' % entry['command']
    if dry_run:
      continue

    exit_code = subprocess.call(entry['command'])
    if exit_code:
      if not final_exit_code:
        final_exit_code = exit_code
      failure_list.append(entry['name'])

  print 72 * '='
  print 'SUMMARY:',
  if dry_run:
    print 'Dry run: no tests run'
  elif not failure_list:
    assert not final_exit_code
    print 'All tests passed'
  else:
    assert final_exit_code
    print 'The following had failures:', ', '.join(failure_list)

  return final_exit_code


def test(config):
  return _run_tests(config, [Config.TEST_TYPE_DEFAULT])


def perftest(config):
  return _run_tests(config, [Config.TEST_TYPE_PERF])


def pytest(config):
  return _run_tests(config, ['python'])


def nacltest(config):
  return _run_tests(config, ['nacl'])


def main():
  os.chdir(Paths().src_root)

  parser = argparse.ArgumentParser(description='A script to make building'
      '/testing Mojo components easier.')

  parent_parser = argparse.ArgumentParser(add_help=False)
  parent_parser.add_argument('--asan', help='Use Address Sanitizer',
                             action='store_true')
  parent_parser.add_argument('--dcheck_always_on',
                             help='DCHECK and MOJO_DCHECK are fatal even in '
                             'release builds',
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

  parent_parser.add_argument('--target-cpu',
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

  nacltest_parser = subparsers.add_parser('nacltest', parents=[parent_parser],
      help='Run NaCl unit tests (does not build).')
  nacltest_parser.set_defaults(func=nacltest)

  dartcheck_parser = subparsers.add_parser('dartcheck', parents=[parent_parser],
      help='Run the dart source code analyzer to check for warnings.')
  dartcheck_parser.set_defaults(func=dartcheck)

  args = parser.parse_args()
  config = _args_to_config(args)
  return args.func(config)


if __name__ == '__main__':
  sys.exit(main())
