#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This a simple script to make building/testing Mojo components easier.

import argparse
import os
import platform
import re
import subprocess
import sys

from mopy.config import Config
from mopy.paths import Paths


def args_to_config(args):
  # Default to host OS.
  target_os = None
  if args.android:
    target_os = Config.OS_ANDROID
  elif args.chromeos:
    target_os = Config.OS_CHROMEOS

  additional_args = {}

  if 'clang' in args:
    if args.clang is not None:
      # If --clang/--gcc is specified, use that.
      additional_args['is_clang'] = args.clang
    else:
      # Otherwise, use the default (use clang for everything except Android and
      # Windows).
      additional_args['is_clang'] = (target_os not in (Config.OS_ANDROID,
                                                       Config.OS_WINDOWS))

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

  if 'builder_name' in args:
    additional_args['builder_name'] = args.builder_name
  if 'build_number' in args:
    additional_args['build_number'] = args.build_number
  if 'master_name' in args:
    additional_args['master_name'] = args.master_name
  if 'test_results_server' in args:
    additional_args['test_results_server'] = args.test_results_server

  return Config(target_os=target_os, is_debug=args.debug, **additional_args)


def get_out_dir(config):
  paths = Paths(config)
  return paths.SrcRelPath(paths.build_dir)


def sync(config):
  # pylint: disable=W0613
  return subprocess.call(['gclient', 'sync'])


def gn(config):
  command = ['gn', 'gen']

  gn_args = []
  gn_args.append('is_debug=' + ('true' if config.is_debug else 'false'))
  gn_args.append('is_asan=' + ('true' if config.sanitizer ==
                               Config.SANITIZER_ASAN else 'false'))
  gn_args.append('is_clang=' + ('true' if config.is_clang else 'false'))

  if config.values["use_goma"]:
    gn_args.append('use_goma=true')
    gn_args.append(r'''goma_dir=\"%s\"''' % config.values["goma_dir"])
  else:
    gn_args.append('use_goma=false')

  if config.values["with_dart"]:
    gn_args.append('mojo_use_dart=true')

  if config.target_os == Config.OS_ANDROID:
    gn_args.append(r'''os=\"android\" cpu_arch=\"arm\"''')
  elif config.target_os == Config.OS_CHROMEOS:
    gn_args.append(r'''os=\"chromeos\" ui_base_build_ime=false
                   use_system_harfbuzz=false''')

  out_dir = get_out_dir(config)
  command.append(out_dir)
  command.append('--args="%s"' % ' '.join(gn_args))

  print 'Running %s ...' % ' '.join(command)
  return subprocess.call(' '.join(command), shell=True)


def get_gn_arg_value(out_dir, arg):
  args_file_path = os.path.join(out_dir, "args.gn")
  if os.path.isfile(args_file_path):
    key_value_regex = re.compile(r'^%s = (.+)$' % arg)
    with open(args_file_path, "r") as args_file:
      for line in args_file.readlines():
        m = key_value_regex.search(line)
        if m:
          return m.group(1).strip('"')
  return ''


def build(config):
  out_dir = get_out_dir(config)
  print 'Building in %s ...' % out_dir
  if get_gn_arg_value(out_dir, 'use_goma') == 'true':
    # Use the configured goma directory.
    local_goma_dir = get_gn_arg_value(out_dir, 'goma_dir')
    print 'Ensuring goma (in %s) started ...' % local_goma_dir
    command = ['python',
               os.path.join(local_goma_dir, 'goma_ctl.py'),
               'ensure_start']
    exit_code = subprocess.call(command)
    if exit_code:
      return exit_code

    return subprocess.call(['ninja', '-j', '1000', '-l', '100', '-C', out_dir,
                            'root'])
  else:
    return subprocess.call(['ninja', '-C', out_dir, 'root'])


def run_testrunner(out_dir, testlist):
  command = ['python']
  if platform.system() == 'Linux':
    command.append('./testing/xvfb.py')
    command.append(out_dir)

  command.append(os.path.join('mojo', 'tools', 'test_runner.py'))
  command.append(os.path.join('mojo', 'tools', 'data', testlist))
  command.append(out_dir)
  command.append('mojob_test_successes')
  return subprocess.call(command)


def run_apptests(config):
  out_dir = get_out_dir(config)
  print 'Running application tests in %s ...' % out_dir
  command = ['python']
  if platform.system() == 'Linux':
    command.append('./testing/xvfb.py')
    command.append(out_dir)

  command.append(os.path.join('mojo', 'tools', 'apptest_runner.py'))
  command.append(os.path.join('mojo', 'tools', 'data', 'apptests'))
  command.append(out_dir)
  return subprocess.call(command)


def run_unittests(config):
  out_dir = get_out_dir(config)
  print 'Running unit tests in %s ...' % out_dir
  return run_testrunner(out_dir, 'unittests')


def run_skytests(config):
  out_dir = get_out_dir(config)
  if platform.system() != 'Linux':
    return 0

  command = []
  command.append('./testing/xvfb.py')
  command.append(out_dir)
  command.append('sky/tools/test_sky')
  command.append('-t')
  command.append('Debug' if config.is_debug else 'Release')
  command.append('--no-new-test-results')
  command.append('--no-show-results')
  command.append('--verbose')

  if config.values["builder_name"]:
    command.append('--builder-name')
    command.append(config.values["builder_name"])

  if config.values["build_number"]:
    command.append('--build-number')
    command.append(config.values["build_number"])

  if config.values["master_name"]:
    command.append('--master-name')
    command.append(config.values["master_name"])

  if config.values["test_results_server"]:
    command.append('--test-results-server')
    command.append(config.values["test_results_server"])

  subprocess.call(command)
  # Sky tests are currently really unstable, so make the step green even if
  # tests actually fail.
  return 0


def run_pytests(config):
  out_dir = get_out_dir(config)
  print 'Running python tests in %s ...' % out_dir
  command = ['python']
  command.append(os.path.join('mojo', 'tools', 'run_mojo_python_tests.py'))
  exit_code = subprocess.call(command)
  if exit_code:
    return exit_code

  if platform.system() != 'Linux':
    print ('Python bindings tests are only supported on Linux.')
    return

  command = ['python']
  command.append(os.path.join('mojo', 'tools',
                              'run_mojo_python_bindings_tests.py'))
  command.append('--build-dir=' + out_dir)
  return subprocess.call(command)


def test(config):
  test_suites = [run_unittests, run_apptests, run_pytests, run_skytests]
  final_exit_code = 0

  for test_suite in test_suites:
    exit_code = test_suite(config)
    # TODO(ojan): Find a better way to do this. We want to run all the tests
    # so we get coverage even if an early test suite fails, but we only have
    # one exit code.
    if not final_exit_code:
      final_exit_code = exit_code

  return final_exit_code


def perftest(config):
  out_dir = get_out_dir(config)
  print 'Running perf tests in %s ...' % out_dir
  command = []
  command.append(os.path.join(out_dir, 'mojo_public_system_perftests'))
  return subprocess.call(command)


def pytest(config):
  return run_pytests(config)


def darttest(config):
  out_dir = get_out_dir(config)
  print 'Running Dart tests in %s ...' % out_dir
  exit_code = run_testrunner(out_dir, 'dart_unittests')
  if exit_code:
    return exit_code

  command = []
  command.append('dart')
  command.append('--checked')
  command.append('--enable-async')
  command.append(os.path.join('mojo', 'tools', 'dart_test_runner.dart'))
  command.append(os.path.join(out_dir, 'gen'))
  return subprocess.call(command)


def main():
  os.chdir(Paths().src_root)

  parser = argparse.ArgumentParser(description='A script to make building'
      '/testing Mojo components easier.')

  parent_parser = argparse.ArgumentParser(add_help=False)
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

  subparsers = parser.add_subparsers()

  sync_parser = subparsers.add_parser('sync', parents=[parent_parser],
      help='Sync using gclient (does not run gn).')
  sync_parser.set_defaults(func=sync)

  gn_parser = subparsers.add_parser('gn', parents=[parent_parser],
                                    help='Run gn for mojo (does not sync).')
  gn_parser.set_defaults(func=gn)
  gn_parser.add_argument('--asan', help='Uses Address Sanitizer',
                         action='store_true')
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
                              'exists;default)',
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

  # Arguments required for uploading to the flakiness dashboard.
  test_parser.add_argument('--master-name',
      help='The name of the buildbot master.')
  test_parser.add_argument('--builder-name',
      help=('The name of the builder shown on the waterfall running '
            'this script e.g. Mojo Linux.'))
  test_parser.add_argument('--build-number',
      help='The build number of the builder running this script.')
  test_parser.add_argument('--test-results-server',
      help='Upload results json files to this appengine server.')

  perftest_parser = subparsers.add_parser('perftest', parents=[parent_parser],
      help='Run perf tests (does not build).')
  perftest_parser.set_defaults(func=perftest)

  pytest_parser = subparsers.add_parser('pytest', parents=[parent_parser],
      help='Run Python unit tests (does not build).')
  pytest_parser.set_defaults(func=pytest)

  darttest_parser = subparsers.add_parser('darttest', parents=[parent_parser],
      help='Run Dart unit tests (does not build).')
  darttest_parser.set_defaults(func=darttest)

  args = parser.parse_args()
  config = args_to_config(args)
  return args.func(config)


if __name__ == '__main__':
  sys.exit(main())
