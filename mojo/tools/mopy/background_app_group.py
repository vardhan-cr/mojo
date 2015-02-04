# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess
import time
import sys

import mopy.paths

from shutil import copyfileobj, rmtree
from signal import SIGTERM
from tempfile import mkdtemp, TemporaryFile


class TimeoutError(Exception):
  """Allows distinction between timeout failures and generic OSErrors."""
  pass


def _poll_for_condition(
    condition, max_seconds=10, sleep_interval=0.1, desc='[unnamed condition]'):
  """Poll until a condition becomes true.

  Arguments:
    condition: callable taking no args and returning bool.
    max_seconds: maximum number of seconds to wait.
                 Might bail up to sleep_interval seconds early.
    sleep_interval: number of seconds to sleep between polls.
    desc: description put in TimeoutError.

  Returns:
    The true value that caused the poll loop to terminate.

  Raises:
    TimeoutError if condition doesn't become true before max_seconds is reached.
  """
  start_time = time.time()
  while time.time() + sleep_interval - start_time <= max_seconds:
    value = condition()
    if value:
      return value
    time.sleep(sleep_interval)

  raise TimeoutError('Timed out waiting for condition: %s' % desc)


class _BackgroundShell(object):
  """Manages a mojo_shell instance that listens for external applications."""

  def __init__(self, mojo_shell_path, shell_args=None):
    """In a background process, run a shell at mojo_shell_path listening
    for external apps on an instance-specific socket.

    Arguments:
      mojo_shell_path: path to the mojo_shell binary to run.
      shell_args: a list of arguments to pass to mojo_shell.

    Raises:
      a TimeoutError if the shell takes too long to create the socket.
    """
    self._tempdir = mkdtemp(prefix='background_shell_')
    self._socket_path = os.path.join(self._tempdir, 'socket')
    self._output_file = TemporaryFile()

    shell_command = [mojo_shell_path,
                     '--enable-external-applications=' + self._socket_path]
    if shell_args:
      shell_command += shell_args
    logging.getLogger().debug(shell_command)

    self._shell = subprocess.Popen(shell_command, stdout=self._output_file,
                                   stderr=subprocess.STDOUT)
    _poll_for_condition(lambda: os.path.exists(self._socket_path),
                        desc="External app socket creation.")


  def __del__(self):
    if self._shell:
      self._shell.terminate()
      self._shell.wait()
      if self._shell.returncode != -SIGTERM:
        copyfileobj(self._output_file, sys.stdout)
    rmtree(self._tempdir)


  @property
  def socket_path(self):
    """The path of the socket where the shell is listening for external apps."""
    return self._socket_path


class BackgroundAppGroup(object):
  """Manages a group of mojo apps running in the background."""

  def __init__(self, paths, app_urls, shell_args=None):
    """In a background process, spins up mojo_shell with external
    applications enabled, passing an optional list of extra arguments.
    Then, launches apps indicated by app_urls in the background.
    The apps and shell are automatically torn down upon destruction.

    Arguments:
      paths: a mopy.paths.Paths object.
      app_urls: a list of URLs for apps to run via mojo_launcher.
      shell_args: a list of arguments to pass to mojo_shell.

    Raises:
      a TimeoutError if the shell takes too long to begin running.
    """
    self._shell = _BackgroundShell(paths.mojo_shell_path, shell_args)

    # Run apps defined by app_urls in the background.
    self._apps = []
    for app_url in app_urls:
      launch_command = [
          paths.mojo_launcher_path,
          '--shell-path=' + self._shell.socket_path,
          '--app-url=' + app_url,
          '--app-path=' + paths.FileFromUrl(app_url),
          '--vmodule=*/mojo/shell/*=2']
      logging.getLogger().debug(launch_command)
      app_output_file = TemporaryFile()
      self._apps.append((app_output_file,
                         subprocess.Popen(launch_command,
                                          stdout=app_output_file,
                                          stderr=subprocess.STDOUT)))


  def __del__(self):
    self._StopApps()


  def __enter__(self):
    return self


  def __exit__(self, ex_type, ex_value, traceback):
    self._StopApps()


  def _StopApps(self):
    """Terminate all background apps."""
    for output_file, app in self._apps:
      app.terminate()
      app.wait()
      if app.returncode != -SIGTERM:
        copyfileobj(output_file, sys.stdout)
    self._apps = []


  @property
  def socket_path(self):
    """The path of the socket where the shell is listening for external apps."""
    return self._shell._socket_path
