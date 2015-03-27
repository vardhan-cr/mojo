# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

from fetcher.mojom_file import MojomFile

class FakeRepository(object):
  def get_repo_base_directory(self):
    return "/base/repo"

  def get_external_directory(self):
    return "/base/repo/third_party/external"


class TestMojomFile(unittest.TestCase):
  def test_add_dependency(self):
    mojom = MojomFile(FakeRepository(), "mojom_name")
    mojom.add_dependency("dependency_name")
    self.assertEqual(1, len(mojom.deps))
    self.assertEqual("mojom_name", mojom.deps[0].get_importer())
    self.assertEqual("dependency_name", mojom.deps[0].get_imported())
