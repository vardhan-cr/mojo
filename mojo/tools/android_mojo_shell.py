#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import atexit
import logging
import threading
import os
import sys

import SimpleHTTPServer
import SocketServer

from mopy.config import Config
from mopy.paths import Paths

sys.path.insert(0, os.path.join(Paths().src_root, 'build', 'android'))
from pylib import android_commands
from pylib import constants
from pylib import forwarder

TAGS = [
    'AndroidHandler',
    'MojoMain',
    'MojoShellActivity',
    'MojoShellApplication',
    'chromium',
]

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

def GetHandlerForPath(base_path):
  class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """
    Handler for SocketServer.TCPServer that will serve the files from
    |base_path| directory over http.
    """

    def translate_path(self, path):
      path_from_current = (
          SimpleHTTPServer.SimpleHTTPRequestHandler.translate_path(self, path))
      return os.path.join(base_path, os.path.relpath(path_from_current))

  return RequestHandler

def main():
  logging.basicConfig()

  parser = argparse.ArgumentParser(usage=USAGE)

  debug_group = parser.add_mutually_exclusive_group()
  debug_group.add_argument('--debug', help='Debug build (default)',
                           default=True, action='store_true')
  debug_group.add_argument('--release', help='Release build', default=False,
                           dest='debug', action='store_false')
  launcher_args, args = parser.parse_known_args()

  paths = Paths(Config(target_os=Config.OS_ANDROID,
                       is_debug=launcher_args.debug))
  constants.SetOutputDirectort(paths.build_dir)

  httpd = SocketServer.TCPServer(('127.0.0.1', 0),
                                 GetHandlerForPath(paths.build_dir))
  atexit.register(httpd.shutdown)
  port = httpd.server_address[1]

  http_thread = threading.Thread(target=httpd.serve_forever)
  http_thread.daemon = True
  http_thread.start()

  device = android_commands.AndroidCommands(
      android_commands.GetAttachedDevices()[0])
  device.EnableAdbRoot()

  atexit.register(forwarder.Forwarder.UnmapAllDevicePorts, device)
  forwarder.Forwarder.Map([(0, port)], device)
  device_port = forwarder.Forwarder.DevicePortForHostPort(port)

  cmd = ('am start'
         ' -W'
         ' -S'
         ' -a android.intent.action.VIEW'
         ' -n org.chromium.mojo_shell_apk/.MojoShellActivity')

  parameters = [
      '--origin=http://127.0.0.1:%d/' % device_port
  ]
  parameters += args
  cmd += ' --esa parameters \"%s\"' % ','.join(parameters)

  device.RunShellCommand('logcat -c')
  device.RunShellCommand(cmd)
  os.system("%s logcat -s %s" % (constants.GetAdbPath(), ' '.join(TAGS)))
  device.RunShellCommand("am force-stop org.chromium.mojo_shell_apk")
  return 0

if __name__ == "__main__":
  sys.exit(main())
