# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import subprocess
import time

from mopy.print_process_error import print_process_error


_logger = logging.getLogger()


def build_shell_arguments(shell_args, apps_and_args=None):
  """Build the list of arguments for the shell. |shell_args| are the base
  arguments, |apps_and_args| is a dictionary that associates each application to
  its specific arguments|. Each app included will be run by the shell.
  """
  result = shell_args[:]
  if apps_and_args:
    for (application, args) in apps_and_args.items():
      result += ["--args-for=%s %s" % (application, " ".join(args))]
    result += apps_and_args.keys()
  return result


def build_command_line(shell_args, apps_and_args):
  return "mojo_shell " + " ".join(["%r" % x for x in build_shell_arguments(
      shell_args, apps_and_args)])


def run_test(shell, shell_args, apps_and_args):
  """Runs the given test."""

  arguments = build_shell_arguments(shell_args, apps_and_args)
  _logger.debug("Starting: mojo_shell %s" % " ".join(arguments))
  start_time = time.time()

  (exit_code, output) = shell.RunUntilCompletion(arguments)
  if exit_code:
    command_line = build_command_line(shell_args, apps_and_args)
    raise subprocess.CalledProcessError(exit_code, command_line, output)

  run_time = time.time() - start_time
  _logger.debug("Completed: mojo_shell %s" % " ".join(arguments))
  # Only log if it took more than 3 second.
  if run_time >= 3:
    _logger.info("Test took %.3f seconds: mojo_shell %s" %
                 (run_time, " ".join(arguments)))
  return output


def try_run_test(shell, shell_args, apps_and_args):
  """Returns the output of a shell run or an empty string on error."""
  command_line = build_command_line(shell_args, apps_and_args)
  _logger.debug("Running " + command_line)
  try:
    return run_test(shell, shell_args, apps_and_args)
  except Exception as e:
    print_process_error(command_line, e)
  return None
