# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess

from pylib.shell import Shell


class LinuxShell(Shell):
  """Wrapper around Mojo shell running on Linux.

  Args:
    executable_path: path to the shell binary
    command_prefix: optional list of arguments to prepend to the shell command,
        allowing e.g. to run the shell under debugger.
  """

  def __init__(self, executable_path, command_prefix=None):
    self.executable_path = executable_path
    self.command_prefix = command_prefix if command_prefix else []

  def Run(self, arguments):
    """Runs the shell with given arguments until shell exits, passing the stdout
    mingled with stderr produced by the shell onto the stdout.

    Returns:
      Exit code retured by the shell or None if the exit code cannot be
      retrieved.
    """
    command = self.command_prefix + [self.executable_path] + arguments
    return subprocess.call(command, stderr=subprocess.STDOUT)

  def RunAndGetOutput(self, arguments):
    """Runs the shell with given arguments until shell exits.

    Args:
      arguments: list of arguments for the shell

    Returns:
      A tuple of (return_code, output). |return_code| is the exit code returned
      by the shell or None if the exit code cannot be retrieved. |output| is the
      stdout mingled with the stderr produced by the shell.
    """
    command = self.command_prefix + [self.executable_path] + arguments
    p = subprocess.Popen(command, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
    (output, _) = p.communicate()
    return p.returncode, output
