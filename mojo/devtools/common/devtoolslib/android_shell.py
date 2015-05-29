# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import hashlib
import itertools
import json
import logging
import os
import os.path
import random
import subprocess
import sys
import tempfile
import threading
import time
import urlparse

from devtoolslib.http_server import StartHttpServer
from devtoolslib.shell import Shell


# Tags used by mojo shell Java logging.
LOGCAT_JAVA_TAGS = [
    'AndroidHandler',
    'MojoFileHelper',
    'MojoMain',
    'MojoShellActivity',
    'MojoShellApplication',
]

# Tags used by native logging reflected in the logcat.
LOGCAT_NATIVE_TAGS = [
    'chromium',
]

MOJO_SHELL_PACKAGE_NAME = 'org.chromium.mojo.shell'

MAPPING_PREFIX = '--map-origin='

DEFAULT_BASE_PORT = 31337

_logger = logging.getLogger()


def _IsMapOrigin(arg):
  """Returns whether arg is a --map-origin argument."""
  return arg.startswith(MAPPING_PREFIX)


def _Split(l, pred):
  positive = []
  negative = []
  for v in l:
    if pred(v):
      positive.append(v)
    else:
      negative.append(v)
  return (positive, negative)


def _ExitIfNeeded(process):
  """Exits |process| if it is still alive."""
  if process.poll() is None:
    process.kill()


class AndroidShell(Shell):
  """Wrapper around Mojo shell running on an Android device.

  Args:
    adb_path: Path to adb, optional if adb is in PATH.
    target_device: Device to run on, if multiple devices are connected.
    logcat_tags: Comma-separated list of additional logcat tags to use.
  """

  def __init__(self, adb_path="adb", target_device=None, logcat_tags=None,
               verbose_pipe=None):
    self.adb_path = adb_path
    self.target_device = target_device
    self.stop_shell_registered = False
    self.adb_running_as_root = False
    self.additional_logcat_tags = logcat_tags
    self.verbose_pipe = verbose_pipe if verbose_pipe else open(os.devnull, 'w')

  def _CreateADBCommand(self, args):
    adb_command = [self.adb_path]
    if self.target_device:
      adb_command.extend(['-s', self.target_device])
    adb_command.extend(args)
    return adb_command

  def _ReadFifo(self, fifo_path, pipe, on_fifo_closed, max_attempts=5):
    """Reads |fifo_path| on the device and write the contents to |pipe|. Calls
    |on_fifo_closed| when the fifo is closed. This method will try to find the
    path up to |max_attempts|, waiting 1 second between each attempt. If it
    cannot find |fifo_path|, a exception will be raised.
    """
    fifo_command = self._CreateADBCommand(
        ['shell', 'test -e "%s"; echo $?' % fifo_path])

    def Run():
      def _WaitForFifo():
        for _ in xrange(max_attempts):
          if subprocess.check_output(fifo_command)[0] == '0':
            return
          time.sleep(1)
        if on_fifo_closed:
          on_fifo_closed()
        raise Exception("Unable to find fifo.")
      _WaitForFifo()
      stdout_cat = subprocess.Popen(self._CreateADBCommand([
                                     'shell',
                                     'cat',
                                     fifo_path]),
                                    stdout=pipe)
      atexit.register(_ExitIfNeeded, stdout_cat)
      stdout_cat.wait()
      if on_fifo_closed:
        on_fifo_closed()

    thread = threading.Thread(target=Run, name="StdoutRedirector")
    thread.start()

  def _MapPort(self, device_port, host_port):
    """Maps the device port to the host port. If |device_port| is 0, a random
    available port is chosen. Returns the device port.
    """
    def _FindAvailablePortOnDevice():
      opened = subprocess.check_output(
          self._CreateADBCommand(['shell', 'netstat']))
      opened = [int(x.strip().split()[3].split(':')[1])
                for x in opened if x.startswith(' tcp')]
      while True:
        port = random.randint(4096, 16384)
        if port not in opened:
          return port
    if device_port == 0:
      device_port = _FindAvailablePortOnDevice()
    subprocess.check_call(self._CreateADBCommand([
        "reverse",
        "tcp:%d" % device_port,
        "tcp:%d" % host_port]))

    unmap_command = self._CreateADBCommand(["reverse", "--remove",
                     "tcp:%d" % device_port])

    def _UnmapPort():
      subprocess.Popen(unmap_command)
    atexit.register(_UnmapPort)
    return device_port

  def _StartHttpServerForOriginMapping(self, mapping, port):
    """If |mapping| points at a local file starts an http server to serve files
    from the directory and returns the new mapping.

    This is intended to be called for every --map-origin value.
    """
    parts = mapping.split('=')
    if len(parts) != 2:
      return mapping
    dest = parts[1]
    # If the destination is a url, don't map it.
    if urlparse.urlparse(dest)[0]:
      return mapping
    # Assume the destination is a local file. Start a local server that
    # redirects to it.
    localUrl = self.ServeLocalDirectory(dest, port)
    print 'started server at %s for %s' % (dest, localUrl)
    return parts[0] + '=' + localUrl

  def _StartHttpServerForOriginMappings(self, map_parameters, fixed_port):
    """Calls _StartHttpServerForOriginMapping for every --map-origin
    argument.
    """
    if not map_parameters:
      return []

    original_values = list(itertools.chain(
        *map(lambda x: x[len(MAPPING_PREFIX):].split(','), map_parameters)))
    sorted(original_values)
    result = []
    for i, value in enumerate(original_values):
      result.append(self._StartHttpServerForOriginMapping(
          value, DEFAULT_BASE_PORT + 1 + i if fixed_port else 0))
    return [MAPPING_PREFIX + ','.join(result)]

  def _RunAdbAsRoot(self):
    if self.adb_running_as_root:
      return

    if 'cannot run as root' in subprocess.check_output(
        self._CreateADBCommand(['root'])):
      raise Exception("Unable to run adb as root.")

    # Wait for adbd to restart.
    subprocess.check_call(
        self._CreateADBCommand(['wait-for-device']),
        stdout=self.verbose_pipe)
    self.adb_running_as_root = True

  def _IsShellPackageInstalled(self):
    # Adb should print one line if the package is installed and return empty
    # string otherwise.
    return len(subprocess.check_output(self._CreateADBCommand([
        'shell', 'pm', 'list', 'packages', MOJO_SHELL_PACKAGE_NAME]))) > 0

  def InstallApk(self, shell_apk_path):
    """Installs the apk on the device.

    This method computes checksum of the APK
    and skips the installation if the fingerprint matches the one saved on the
    device upon the previous installation.

    Args:
      shell_apk_path: Path to the shell Android binary.
    """
    device_sha1_path = '/sdcard/%s/%s.sha1' % (MOJO_SHELL_PACKAGE_NAME,
                                               'MojoShell')
    apk_sha1 = hashlib.sha1(open(shell_apk_path, 'rb').read()).hexdigest()
    device_apk_sha1 = subprocess.check_output(self._CreateADBCommand([
        'shell', 'cat', device_sha1_path]))
    do_install = (apk_sha1 != device_apk_sha1 or
                  not self._IsShellPackageInstalled())

    if do_install:
      subprocess.check_call(
          self._CreateADBCommand(['install', '-r', shell_apk_path, '-i',
                                  MOJO_SHELL_PACKAGE_NAME]),
          stdout=self.verbose_pipe)

      # Update the stamp on the device.
      with tempfile.NamedTemporaryFile() as fp:
        fp.write(apk_sha1)
        fp.flush()
        subprocess.check_call(self._CreateADBCommand(['push', fp.name,
                                                      device_sha1_path]),
                              stdout=self.verbose_pipe)
    else:
      # To ensure predictable state after running InstallApk(), we need to stop
      # the shell here, as this is what "adb install" implicitly does.
      self.StopShell()

  def SetUpLocalOrigin(self, local_dir, fixed_port=True):
    """Sets up a local http server to serve files in |local_dir| along with
    device port forwarding. Returns the origin flag to be set when running the
    shell.
    """

    origin_url = self.ServeLocalDirectory(
        local_dir, DEFAULT_BASE_PORT if fixed_port else 0)
    return "--origin=" + origin_url

  def ServeLocalDirectory(self, local_dir_path, port=0):
    """Serves the content of the local (host) directory, making it available to
    the shell under the url returned by the function.

    The server will run on a separate thread until the program terminates. The
    call returns immediately.

    Args:
      local_dir_path: path to the directory to be served
      port: port at which the server will be available to the shell

    Returns:
      The url that the shell can use to access the content of |local_dir_path|.
    """
    assert local_dir_path
    print 'starting http for', local_dir_path
    server_address = StartHttpServer(local_dir_path)

    print 'local port=%d' % server_address[1]
    return 'http://127.0.0.1:%d/' % self._MapPort(port,
                                                  server_address[1])

  def StartShell(self,
                 arguments,
                 stdout=None,
                 on_application_stop=None,
                 fixed_port=True):
    """Starts the mojo shell, passing it the given arguments.

    The |arguments| list must contain the "--origin=" arg. SetUpLocalOrigin()
    can be used to set up a local directory on the host machine as origin.
    If |stdout| is not None, it should be a valid argument for subprocess.Popen.
    """
    if not self.stop_shell_registered:
      atexit.register(self.StopShell)
      self.stop_shell_registered = True

    STDOUT_PIPE = "/data/data/%s/stdout.fifo" % MOJO_SHELL_PACKAGE_NAME

    cmd = self._CreateADBCommand([
           'shell',
           'am',
           'start',
           '-S',
           '-a', 'android.intent.action.VIEW',
           '-n', '%s/.MojoShellActivity' % MOJO_SHELL_PACKAGE_NAME])

    parameters = []
    if stdout or on_application_stop:
      # We need to run as root to access the fifo file we use for stdout
      # redirection.
      self._RunAdbAsRoot()

      # Remove any leftover fifo file after the previous run.
      subprocess.check_call(self._CreateADBCommand(
          ['shell', 'rm', '-f', STDOUT_PIPE]))

      parameters.append('--fifo-path=%s' % STDOUT_PIPE)
      self._ReadFifo(STDOUT_PIPE, stdout, on_application_stop)
    # The origin has to be specified whether it's local or external.
    assert any("--origin=" in arg for arg in arguments)

    # Extract map-origin arguments.
    map_parameters, other_parameters = _Split(arguments, _IsMapOrigin)
    parameters += other_parameters
    parameters += self._StartHttpServerForOriginMappings(map_parameters,
                                                         fixed_port)

    if parameters:
      encodedParameters = json.dumps(parameters)
      cmd += ['--es', 'encodedParameters', encodedParameters]

    subprocess.check_call(cmd, stdout=self.verbose_pipe)

  def Run(self, arguments):
    """Runs the shell with given arguments until shell exits, passing the stdout
    mingled with stderr produced by the shell onto the stdout.

    Returns:
      Exit code retured by the shell or None if the exit code cannot be
      retrieved.
    """
    self.CleanLogs()
    # Don't carry over the native logs from logcat - we have these in the
    # stdout.
    p = self.ShowLogs(include_native_logs=False)
    self.StartShell(arguments, sys.stdout, p.terminate)
    p.wait()
    return None

  def RunAndGetOutput(self, arguments):
    """Runs the shell with given arguments until shell exits.

    Args:
      arguments: list of arguments for the shell

    Returns:
      A tuple of (return_code, output). |return_code| is the exit code returned
      by the shell or None if the exit code cannot be retrieved. |output| is the
      stdout mingled with the stderr produced by the shell.
    """
    (r, w) = os.pipe()
    with os.fdopen(r, "r") as rf:
      with os.fdopen(w, "w") as wf:
        self.StartShell(arguments, wf, wf.close, False)
        output = rf.read()
        return None, output

  def StopShell(self):
    """Stops the mojo shell."""
    subprocess.check_call(self._CreateADBCommand(['shell',
                                                  'am',
                                                  'force-stop',
                                                  MOJO_SHELL_PACKAGE_NAME]))

  def CleanLogs(self):
    """Cleans the logs on the device."""
    subprocess.check_call(self._CreateADBCommand(['logcat', '-c']))

  def ShowLogs(self, include_native_logs=True):
    """Displays the log for the mojo shell.

    Returns:
      The process responsible for reading the logs.
    """
    tags = LOGCAT_JAVA_TAGS
    if include_native_logs:
      tags.extend(LOGCAT_NATIVE_TAGS)
    if self.additional_logcat_tags is not None:
      tags.extend(self.additional_logcat_tags.split(","))
    logcat = subprocess.Popen(
        self._CreateADBCommand(['logcat',
                                '-s',
                                ' '.join(tags)]),
        stdout=sys.stdout)
    atexit.register(_ExitIfNeeded, logcat)
    return logcat
