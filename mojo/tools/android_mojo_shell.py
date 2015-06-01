#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import sys
import urlparse

import devtools
devtools.add_lib_to_path()
from devtoolslib.android_shell import AndroidShell

from mopy.config import Config
from mopy.paths import Paths

USAGE = ("android_mojo_shell.py "
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

_MAPPING_PREFIX = '--map-origin='
# When spinning up servers for local origins, we want to use predictable ports
# so that caching works between subsequent runs with the same command line.
_MAP_ORIGIN_BASE_PORT = 31338


def _IsMapOrigin(arg):
  """Returns whether |arg| is a --map-origin argument."""
  return arg.startswith(_MAPPING_PREFIX)


def _Split(l, pred):
  positive = []
  negative = []
  for v in l:
    if pred(v):
      positive.append(v)
    else:
      negative.append(v)
  return (positive, negative)


def _RewriteMapOriginParameter(shell, mapping, device_port):
  parts = mapping[len(_MAPPING_PREFIX):].split('=')
  if len(parts) != 2:
    return mapping
  dest = parts[1]
  # If the destination is a url, don't map it.
  if urlparse.urlparse(dest)[0]:
    return mapping
  # Assume the destination is a local directory and serve it.
  localUrl = shell.ServeLocalDirectory(dest, device_port)
  print 'started server at %s for %s' % (dest, localUrl)
  return _MAPPING_PREFIX + parts[0] + '=' + localUrl


def main():
  logging.basicConfig()

  parser = argparse.ArgumentParser(usage=USAGE)

  debug_group = parser.add_mutually_exclusive_group()
  debug_group.add_argument('--debug', help='Debug build (default)',
                           default=True, action='store_true')
  debug_group.add_argument('--release', help='Release build', default=False,
                           dest='debug', action='store_false')
  parser.add_argument('--target-cpu', help='CPU architecture to run for.',
                      choices=['x64', 'x86', 'arm'])
  parser.add_argument('--origin', help='Origin for mojo: URLs.')
  parser.add_argument('--target-device', help='Device to run on.')
  parser.add_argument('--logcat-tags', help='Comma-separated list of '
                      'additional logcat tags to display on the console.')
  parser.add_argument('-v', '--verbose', action="store_true",
                      help="Increase output verbosity")
  launcher_args, args = parser.parse_known_args()

  config = Config(target_os=Config.OS_ANDROID,
                  target_cpu=launcher_args.target_cpu,
                  is_debug=launcher_args.debug)
  paths = Paths(config)
  verbose_pipe = sys.stdout if launcher_args.verbose else None
  shell = AndroidShell(paths.adb_path, launcher_args.target_device,
                       logcat_tags=launcher_args.logcat_tags,
                       verbose_pipe=verbose_pipe)
  shell.InstallApk(paths.target_mojo_shell_path)
  args.append("--origin=" + launcher_args.origin if launcher_args.origin else
              shell.SetUpLocalOrigin(paths.build_dir))

  # Serve the local destinations indicated in map-origin arguments and rewrite
  # the arguments to point to server urls.
  map_parameters, other_parameters = _Split(args, _IsMapOrigin)
  parameters = other_parameters
  next_port = _MAP_ORIGIN_BASE_PORT
  for mapping in sorted(map_parameters):
    parameters.append(_RewriteMapOriginParameter(shell, mapping, next_port))
    next_port += 1
  shell.Run(parameters)
  return 0


if __name__ == "__main__":
  sys.exit(main())
