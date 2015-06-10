#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import sys
import os.path

import devtools
devtools.add_lib_to_path()
from devtoolslib.android_shell import AndroidShell
from devtoolslib.linux_shell import LinuxShell
from devtoolslib import shell_arguments

from mopy.config import Config
from mopy.paths import Paths

USAGE = ("mojo_shell.py "
         "[--args-for=<mojo-app>] "
         "[--content-handlers=<handlers>] "
         "[--enable-external-applications] "
         "[--disable-cache] "
         "[--enable-multiprocess] "
         "[--url-mappings=from1=to1,from2=to2] "
         "[<mojo-app>] "
         """

A <mojo-app> is a Mojo URL or a Mojo URL and arguments within quotes.
Example: mojo_shell "mojo:js_standalone test.js".
<url-lib-path> is searched for shared libraries named by mojo URLs.
The value of <handlers> is a comma separated list like:
text/html,mojo:html_viewer,application/javascript,mojo:js_content_handler
""")

_DEFAULT_WINDOW_MANAGER = "mojo:kiosk_wm"
_SKY_SERVER_PORT = 9998


def main():
  logging.basicConfig()

  parser = argparse.ArgumentParser(usage=USAGE)

  # Arguments indicating the configuration we are targeting.
  parser.add_argument('--android', help='Run on Android',
                      action='store_true')
  debug_group = parser.add_mutually_exclusive_group()
  debug_group.add_argument('--debug', help='Debug build (default)',
                           default=True, action='store_true')
  debug_group.add_argument('--release', help='Release build', default=False,
                           dest='debug', action='store_false')
  parser.add_argument('--target-cpu', help='CPU architecture to run for.',
                      choices=['x64', 'x86', 'arm'])

  # Arguments configuring the shell run.
  parser.add_argument('--origin', help='Origin for mojo: URLs.')
  parser.add_argument('--window-manager', default=_DEFAULT_WINDOW_MANAGER,
                      help='Window manager app to be mapped as '
                      'mojo:window_manager. By default it is ' +
                      _DEFAULT_WINDOW_MANAGER)
  parser.add_argument('--no-debugger', action="store_true",
                      help='Do not spawn mojo:debugger.')
  parser.add_argument('--sky',
                      help='Loads the given Sky file.')
  parser.add_argument('-v', '--verbose', action="store_true",
                      help="Increase output verbosity")

  # Android-only arguments.
  parser.add_argument('--target-device', help='Device to run on.')
  parser.add_argument('--logcat-tags', help='Comma-separated list of '
                      'additional logcat tags to display on the console.')

  launcher_args, args = parser.parse_known_args()
  if launcher_args.android:
    config = Config(target_os=Config.OS_ANDROID,
                    target_cpu=launcher_args.target_cpu,
                    is_debug=launcher_args.debug)
    paths = Paths(config)
    verbose_pipe = sys.stdout if launcher_args.verbose else None

    shell = AndroidShell(paths.adb_path, launcher_args.target_device,
                         logcat_tags=launcher_args.logcat_tags,
                         verbose_pipe=verbose_pipe)
    device_status, error = shell.CheckDevice()
    if not device_status:
      print 'Device check failed: ' + error
      return 1
    shell.InstallApk(paths.target_mojo_shell_path)

    args = shell_arguments.RewriteMapOriginParameters(shell, args)
    if not launcher_args.origin:
      args.extend(shell_arguments.ConfigureLocalOrigin(shell, paths.build_dir))
  else:
    config = Config(target_os=Config.OS_LINUX,
                    target_cpu=launcher_args.target_cpu,
                    is_debug=launcher_args.debug)
    paths = Paths(config)
    shell = LinuxShell(paths.mojo_shell_path)

  if launcher_args.origin:
    args.append('--origin=' + launcher_args.origin)
  args = shell_arguments.AppendToArgument(args, '--url-mappings=',
                                          'mojo:window_manager=%s' %
                                          launcher_args.window_manager)
  if not launcher_args.no_debugger:
    args.extend(shell_arguments.ConfigureDebugger(shell))

  if launcher_args.sky:
    # Configure a server to serve the checkout root at / (so that Sky examples
    # are accessible using a root-relative path) and Sky packages at /packages.
    # This is independent from the server that potentially serves the origin
    # directory containing the mojo: apps.
    packages_local_path = os.path.join(paths.build_dir, 'gen', 'dart-pkg',
                                       'packages')
    additional_mappings = [
        ('packages/', packages_local_path),
    ]
    server_url = shell.ServeLocalDirectory(paths.src_root,
        port=_SKY_SERVER_PORT, additional_mappings=additional_mappings)

    # Configure the content type mappings for the sky_viewer. This is needed
    # only for the Sky apps that do not declare mojo:sky_viewer in a shebang,
    # and it is unfortunate as it configures the shell to map all items of the
    # application/dart content-type as Sky apps.
    # TODO(ppi): drop this part once we can rely on the Sky files declaring
    # correct shebang.
    args = shell_arguments.AppendToArgument(args, '--content-handlers=',
                                            'text/sky,mojo:sky_viewer')
    args = shell_arguments.AppendToArgument(args, '--content-handlers=',
                                            'application/dart,mojo:sky_viewer')

    # Configure the window manager to embed the sky_viewer.
    sky_url = server_url + launcher_args.sky
    args.append('mojo:window_manager %s' % sky_url)

  if launcher_args.verbose:
    print "Shell arguments: " + str(args)

  shell.Run(args)
  return 0


if __name__ == "__main__":
  sys.exit(main())
