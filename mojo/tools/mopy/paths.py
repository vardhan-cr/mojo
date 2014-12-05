# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from .config import Config

class Paths(object):
  """Provides commonly used paths"""

  def __init__(self, config=None, build_dir=None):
    """Specify either a config or a build_dir to generate paths to binary
    artifacts."""
    self.src_root = os.path.abspath(os.path.join(__file__,
      os.pardir, os.pardir, os.pardir, os.pardir))
    self.mojo_dir = os.path.join(self.src_root, "mojo")

    if config:
      assert build_dir is None
      subdir = ""
      if config.target_os == Config.OS_ANDROID:
        subdir += "android_"
      elif config.target_os == Config.OS_CHROMEOS:
        subdir += "chromeos_"
      subdir += "Debug" if config.is_debug else "Release"
      if config.sanitizer == Config.SANITIZER_ASAN:
        subdir += "_asan"
      self.build_dir = os.path.join(self.src_root, "out", subdir)
    elif build_dir is not None:
      self.build_dir = os.path.abspath(build_dir)
    else:
      self.build_dir = None

    if self.build_dir is not None:
      self.mojo_shell_path = os.path.join(self.build_dir, "mojo_shell")
      # TODO(vtl): Use the host OS here, since |config| may not be available.
      # In any case, if the target is Windows, but the host isn't, using
      # |os.path| isn't correct....
      if Config.GetHostOS() == Config.OS_WINDOWS:
        self.mojo_shell_path += ".exe"
    else:
      self.mojo_shell_path = None

  def RelPath(self, path):
    """Returns the given path, relative to the current directory."""
    return os.path.relpath(path)

  def SrcRelPath(self, path):
    """Returns the given path, relative to self.src_root."""
    return os.path.relpath(path, self.src_root)
