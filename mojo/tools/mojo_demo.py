#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from mopy.config import Config
import argparse
import mopy.paths
import os
import pipes
import subprocess
import sys

# FIXME: We need to merge the mojo/tools and sky/tools directories
sys.path.append(os.path.join(mopy.paths.Paths().src_root, 'sky', 'tools'))
from skypy.skyserver import SkyServer
import skypy.paths


def main():
  parser = argparse.ArgumentParser(description='Helper to launch mojo demos')
  parser.add_argument('-d', dest='build_dir', type=str)
  parser.add_argument('--browser', action='store_const', const='browser',
      dest='demo', help='Use the browser demo')
  parser.add_argument('--wm_flow', action='store_const', const='wm_flow',
    dest='demo', help='Use the wm_flow demo')

  args = parser.parse_args()

  config = Config(target_os=Config.OS_LINUX, is_debug=True)
  paths = mopy.paths.Paths(config, build_dir=args.build_dir)
  mojo_shell = paths.mojo_shell_path

  cmd = [mojo_shell]
  cmd.append('--v=1')

  HTTP_PORT = 9999
  configuration = 'Debug' if config.is_debug else 'Release'
  server = SkyServer(HTTP_PORT, configuration, paths.src_root)

  if args.demo == 'browser':
    base_url = server.path_as_url(paths.build_dir)
    wm_url = os.path.join(base_url, 'example_window_manager.mojo')
    browser_url = os.path.join(base_url, 'browser.mojo')
    cmd.append('--url-mappings=mojo:window_manager=mojo:example_window_manager')
    cmd.append('--args-for=mojo:window_manager %s' % (wm_url))
    cmd.append('--args-for=mojo:browser %s' % (browser_url))
    cmd.append('mojo:window_manager')
  elif args.demo == 'wm_flow':
    base_url = server.path_as_url(paths.build_dir)
    wm_url = os.path.join(base_url, 'wm_flow_wm.mojo')
    app_url = os.path.join(base_url, 'wm_flow_app.mojo')
    cmd.append('--url-mappings=mojo:window_manager=' + wm_url)
    # Mojo apps don't know their own URL yet:
    # https://docs.google.com/a/chromium.org/document/d/1AQ2y6ekzvbdaMF5WrUQmneyXJnke-MnYYL4Gz1AKDos
    cmd.append('--args-for=%s %s' % (app_url, app_url))
    cmd.append('--args-for=mojo:window_manager %s' % (wm_url))
    cmd.append(app_url)
  else:
    parser.print_usage()
    print "--browser or --wm_flow is required"
    return 1

  # http://stackoverflow.com/questions/4748344/whats-the-reverse-of-shlex-split
  # shlex.quote doesn't exist until 3.3
  # This doesn't print exactly what we want, but it's better than nothing:
  print " ".join(map(pipes.quote, cmd))
  with server:
    return subprocess.call(cmd)

if __name__ == '__main__':
  sys.exit(main())
