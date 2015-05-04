# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import subprocess

from pylib.shell import Shell


class LinuxShell(Shell):
  """Wrapper around Mojo shell running on Linux.

  Args:
    executable_path: path to the shell binary
  """

  def __init__(self, executable_path):
    self.executable_path = executable_path

  def RunUntilCompletion(self, arguments):
    """Runs the shell with given arguments until shell exits.

    Args:
      arguments: list of arguments for the shell

    Returns:
      A tuple of (return_code, output). |return_code| is the exit code returned
      by the shell or None if the exit code cannot be retrieved. |output| is the
      stdout mingled with the stderr produced by the shell.
    """
    p = subprocess.Popen([self.executable_path] + arguments,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    (output, _) = p.communicate()
    return p.returncode, output
