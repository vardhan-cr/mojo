# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Build/test configurations, which are just dictionaries. This
"defines" the schema and provides some wrappers."""


import json
import os.path
import platform
import sys


class Config(object):
  """A Config is basically just a wrapper around a dictionary that species a
  build/test configuration. The dictionary is accessible through the values
  member."""

  # Valid values for target_os (None is also valid):
  OS_ANDROID = "android"
  OS_CHROMEOS = "chromeos"
  OS_LINUX = "linux"
  OS_MAC = "mac"
  OS_WINDOWS = "windows"

  # Valid values for target_arch (None is also valid):
  ARCH_X86 = "x86"
  ARCH_X64 = "x64"
  ARCH_ARM = "arm"

  # Valid values for sanitizer (None is also valid):
  SANITIZER_ASAN = "asan"

  # Standard values for test types (test types are arbitrary strings; other
  # values are allowed).
  TEST_TYPE_DEFAULT = "default"
  TEST_TYPE_UNIT = "unit"
  TEST_TYPE_PERF = "perf"
  TEST_TYPE_INTEGRATION = "integration"

  def __init__(self, target_os=None, target_arch=None, is_debug=True,
               is_clang=None, sanitizer=None, **kwargs):
    """Constructs a Config with key-value pairs specified via keyword arguments.
    If target_os is not specified, it will be set to the host OS."""

    assert target_os in (None, Config.OS_ANDROID, Config.OS_CHROMEOS,
                         Config.OS_LINUX, Config.OS_MAC, Config.OS_WINDOWS)
    assert target_arch in (None, Config.ARCH_X86, Config.ARCH_X64,
                           Config.ARCH_ARM)
    assert isinstance(is_debug, bool)
    assert is_clang is None or isinstance(is_clang, bool)
    assert sanitizer in (None, Config.SANITIZER_ASAN)
    if "test_types" in kwargs:
      assert isinstance(kwargs["test_types"], list)

    self.values = {}
    self.values["target_os"] = (self.GetHostOS() if target_os is None else
                                target_os)
    self.values["target_arch"] = (self.GetHostCPUArch() if target_arch is None
                                  else target_arch)
    self.values["is_debug"] = is_debug
    self.values["is_clang"] = is_clang
    self.values["sanitizer"] = sanitizer

    self.values.update(kwargs)

  @staticmethod
  def GetHostOS():
    if sys.platform == "linux2":
      return Config.OS_LINUX
    if sys.platform == "darwin":
      return Config.OS_MAC
    if sys.platform == "win32":
      return Config.OS_WINDOWS
    raise NotImplementedError("Unsupported host OS")

  @staticmethod
  def GetHostCPUArch():
    # Derived from //native_client/pynacl/platform.py
    machine = platform.machine()
    if machine in ("x86", "x86-32", "x86_32", "x8632", "i386", "i686", "ia32",
                   "32"):
      return Config.ARCH_X86
    if machine in ("x86-64", "amd64", "x86_64", "x8664", "64"):
      return Config.ARCH_X64
    if machine.startswith("arm"):
      return Config.ARCH_ARM
    raise Exception("Cannot identify CPU arch: %s" % machine)

  # Getters for standard fields ------------------------------------------------

  @property
  def target_os(self):
    """OS of the build/test target."""
    return self.values["target_os"]

  @property
  def target_arch(self):
    """CPU arch of the build/test target."""
    return self.values["target_arch"]

  @property
  def is_debug(self):
    """Is Debug build?"""
    return self.values["is_debug"]

  @property
  def is_clang(self):
    """Should use clang?"""
    return self.values["is_clang"]

  @property
  def sanitizer(self):
    """Sanitizer to use, if any."""
    return self.values["sanitizer"]

  @property
  def test_types(self):
    """List of test types to run."""
    return self.values.get("test_types", [Config.TEST_TYPE_DEFAULT])

  # Utility methods ------------------------------------------------------------

  def match_target_os(self, selector):
    """Returns whether the current config matches the target os selector.

    |selector| is a list of strings. Each string is either:
    *          : matches everything.
    target_os  : matches the given target os.
    !*         : rejects everything
    !target_os : rejects the given target os.

    The last matching selector is considered. If no selector match, the config
    matches.
    """
    for match in reversed(selector):
      invert = match[0] == "!"
      if invert:
        match = match[1:]
      if match == "*" or self.target_os == match:
        return not invert
    return True
