# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import json
import logging
import os
import os.path
import subprocess
import sys
import threading
import time

import SimpleHTTPServer
import SocketServer

from mopy.config import Config
from mopy.paths import Paths

sys.path.insert(0, os.path.join(Paths().src_root, 'build', 'android'))
from pylib import android_commands
from pylib import constants
from pylib import forwarder


# Tags used by the mojo shell application logs.
LOGCAT_TAGS = [
    'AndroidHandler',
    'MojoFileHelper',
    'MojoMain',
    'MojoShellActivity',
    'MojoShellApplication',
    'chromium',
]

MOJO_SHELL_PACKAGE_NAME = 'org.chromium.mojo.shell'


class Context(object):
  """
  The context object allowing to run multiple runs of the shell.
  """
  def __init__(self, device, device_port):
    self.device = device
    self.device_port = device_port


class _SilentTCPServer(SocketServer.TCPServer):
  """
  A TCPServer that won't display any error, unless debugging is enabled. This is
  useful because the client might stop while it is fetching an URL, which causes
  spurious error messages.
  """
  def handle_error(self, request, client_address):
    """
    Override the base class method to have conditional logging.
    """
    if logging.getLogger().isEnabledFor(logging.DEBUG):
      super(_SilentTCPServer, self).handle_error(request, client_address)


def _GetHandlerClassForPath(base_path):
  class RequestHandler(SimpleHTTPServer.SimpleHTTPRequestHandler):
    """
    Handler for SocketServer.TCPServer that will serve the files from
    |base_path| directory over http.
    """

    def translate_path(self, path):
      path_from_current = (
          SimpleHTTPServer.SimpleHTTPRequestHandler.translate_path(self, path))
      return os.path.join(base_path, os.path.relpath(path_from_current))

    def log_message(self, *_):
      """
      Override the base class method to disable logging.
      """
      pass

  return RequestHandler


def _ExitIfNeeded(process):
  """
  Exits |process| if it is still alive.
  """
  if process.poll() is None:
    process.kill()


def _ReadFifo(context, fifo_path, pipe, on_fifo_closed, max_attempts=5):
  """
  Reads |fifo_path| on the device and write the contents to |pipe|. Calls
  |on_fifo_closed| when the fifo is closed. This method will try to find the
  path up to |max_attempts|, waiting 1 second between each attempt. If it cannot
  find |fifo_path|, a exception will be raised.
  """
  def Run():
    def _WaitForFifo():
      for _ in xrange(max_attempts):
        if context.device.FileExistsOnDevice(fifo_path):
          return
        time.sleep(1)
      if on_fifo_closed:
        on_fifo_closed()
      raise Exception("Unable to find fifo.")
    _WaitForFifo()
    stdout_cat = subprocess.Popen([constants.GetAdbPath(),
                                   'shell',
                                   'cat',
                                   fifo_path],
                                  stdout=pipe)
    atexit.register(_ExitIfNeeded, stdout_cat)
    stdout_cat.wait()
    if on_fifo_closed:
      on_fifo_closed()

  thread = threading.Thread(target=Run, name="StdoutRedirector")
  thread.start()


def PrepareShellRun(config):
  """
  Returns a context allowing a shell to be run.

  This will start an internal http server to serve mojo applications, forward a
  local port on the device to this http server and install the current version
  of the mojo shell.
  """
  build_dir = Paths(config).build_dir
  constants.SetOutputDirectort(build_dir)

  httpd = _SilentTCPServer(('127.0.0.1', 0), _GetHandlerClassForPath(build_dir))
  atexit.register(httpd.shutdown)
  host_port = httpd.server_address[1]

  http_thread = threading.Thread(target=httpd.serve_forever)
  http_thread.daemon = True
  http_thread.start()

  device = android_commands.AndroidCommands(
      android_commands.GetAttachedDevices()[0])
  device.EnableAdbRoot()

  device.ManagedInstall(os.path.join(build_dir, 'apks', 'MojoShell.apk'),
                        keep_data=True,
                        package_name=MOJO_SHELL_PACKAGE_NAME)

  atexit.register(forwarder.Forwarder.UnmapAllDevicePorts, device)
  forwarder.Forwarder.Map([(0, host_port)], device)
  context = Context(device,
                    forwarder.Forwarder.DevicePortForHostPort(host_port))

  atexit.register(StopShell, context)
  return context


def StartShell(context, arguments, stdout=None, on_application_stop=None):
  """
  Starts the mojo shell, passing it the given arguments.

  If stdout is not None, it should be a valid argument for subprocess.Popen.
  """
  STDOUT_PIPE = "/data/data/%s/stdout.fifo" % MOJO_SHELL_PACKAGE_NAME

  cmd = [constants.GetAdbPath(),
         'shell',
         'am',
         'start',
         '-W',
         '-S',
         '-a', 'android.intent.action.VIEW',
         '-n', '%s/.MojoShellActivity' % MOJO_SHELL_PACKAGE_NAME]

  parameters = ['--origin=http://127.0.0.1:%d/' % context.device_port]
  if stdout or on_application_stop:
    context.device.RunShellCommand('rm %s' % STDOUT_PIPE)
    parameters.append('--fifo-path=%s' % STDOUT_PIPE)
    _ReadFifo(context, STDOUT_PIPE, stdout, on_application_stop)
  parameters += arguments

  if parameters:
    encodedParameters = json.dumps(parameters)
    cmd += [ '--es', 'encodedParameters', encodedParameters]

  with open(os.devnull, 'w') as devnull:
    subprocess.Popen(cmd, stdout=devnull).wait()


def StopShell(context):
  """
  Stops the mojo shell.
  """
  context.device.RunShellCommand('am force-stop %s' % MOJO_SHELL_PACKAGE_NAME)

def CleanLogs(context):
  """
  Cleans the logs on the device.
  """
  context.device.RunShellCommand('logcat -c')


def ShowLogs():
  """
  Displays the log for the mojo shell.

  Returns the process responsible for reading the logs.
  """
  logcat = subprocess.Popen([constants.GetAdbPath(),
                             'logcat',
                             '-s',
                             ' '.join(LOGCAT_TAGS)],
                            stdout=sys.stdout)
  atexit.register(_ExitIfNeeded, logcat)
  return logcat


def GetFilePath(filename):
  """
  Returns a path suitable for the application to create a file.
  """
  return '/data/data/%s/files/%s' % (MOJO_SHELL_PACKAGE_NAME, filename)
