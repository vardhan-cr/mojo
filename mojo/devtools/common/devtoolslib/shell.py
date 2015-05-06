# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


class Shell(object):
  """Represents an abstract Mojo shell."""

  def Run(self, arguments):
    """Runs the shell with given arguments until shell exits, passing the stdout
    mingled with stderr produced by the shell onto the stdout.

    Returns:
      Exit code retured by the shell or None if the exit code cannot be
      retrieved.
    """
    raise NotImplementedError()

  def RunAndGetOutput(self, arguments):
    """Runs the shell with given arguments until shell exits and returns the
    output.

    Args:
      arguments: list of arguments for the shell

    Returns:
      A tuple of (return_code, output). |return_code| is the exit code returned
      by the shell or None if the exit code cannot be retrieved. |output| is the
      stdout mingled with the stderr produced by the shell.
    """
    raise NotImplementedError()
