#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import sys
import unittest

import PRESUBMIT

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from PRESUBMIT_test_mocks import MockFile
from PRESUBMIT_test_mocks import MockInputApi, MockOutputApi

_SDK_BUILD_FILE = 'mojo/public/some/path/BUILD.gn'
_EDK_BUILD_FILE = 'mojo/edk/some/path/BUILD.gn'
_IRRELEVANT_BUILD_FILE = 'mojo/foo/some/path/BUILD.gn'

class AbsoluteReferencesInBuildFilesTest(unittest.TestCase):
  """Tests the checking for illegal absolute paths within SDK/EDK buildfiles.
  """
  def setUp(self):
    self.sdk_absolute_path = '//mojo/public/some/absolute/path'
    self.sdk_relative_path = 'mojo/public/some/relative/path'
    self.edk_absolute_path = '//mojo/edk/some/absolute/path'
    self.edk_relative_path = 'mojo/edk/some/relative/path'
    self.whitelisted_external_path = '//testing/gtest'
    self.non_whitelisted_external_path = '//base'

  def inputApiContainingFileWithPaths(self, filename, paths):
    """Returns a MockInputApi object with a single file having |filename| as
    its name and |paths| as its contents, with each path being wrapped in a
    pair of double-quotes to match the syntax for strings within BUILD.gn
    files."""
    contents = [ '"%s"' % path for path in paths ]
    mock_file = MockFile(filename, contents)
    mock_input_api = MockInputApi()
    mock_input_api.files.append(mock_file)
    return mock_input_api

  def checkWarningWithSingleItem(self,
                                 warning,
                                 expected_message,
                                 build_file,
                                 line_num,
                                 referenced_path):
    """Checks that |warning| has a message of |expected_message| and a single
    item whose contents are the absolute path warning item for
    (build_file, line_num, referenced_path)."""
    self.assertEqual(expected_message, warning.message)
    self.assertEqual(1, len(warning.items))
    expected_item = PRESUBMIT._PathReferenceInBuildFileWarningItem(
        build_file, line_num, referenced_path)
    self.assertEqual(expected_item, warning.items[0])

  def checkSDKAbsolutePathWarningWithSingleItem(self,
                                                warning,
                                                package,
                                                build_file,
                                                line_num,
                                                referenced_path):
    """Checks that |warning| has the message for an absolute SDK path within
    |package| and a single item whose contents are the absolute path warning
    item for (build_file, line_num, referenced_path)."""
    expected_message = \
        PRESUBMIT._ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGES[package]
    self.checkWarningWithSingleItem(warning,
                                    expected_message,
                                    build_file,
                                    line_num,
                                    referenced_path)

  def testAbsoluteSDKReferenceInSDKBuildFile(self):
    """Tests that an absolute SDK path within an SDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _SDK_BUILD_FILE,
        [ self.sdk_relative_path, self.sdk_absolute_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    self.checkSDKAbsolutePathWarningWithSingleItem(warnings[0],
                                                   'SDK',
                                                   _SDK_BUILD_FILE,
                                                   2,
                                                   self.sdk_absolute_path)

  def testExternalReferenceInSDKBuildFile(self):
    """Tests that an illegal external path in an SDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _SDK_BUILD_FILE,
        [ self.non_whitelisted_external_path, self.whitelisted_external_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    expected_message = PRESUBMIT._ILLEGAL_EXTERNAL_PATH_WARNING_MESSAGE
    self.checkWarningWithSingleItem(warnings[0],
                                    expected_message,
                                    _SDK_BUILD_FILE,
                                    1,
                                    self.non_whitelisted_external_path)

  def testAbsoluteEDKReferenceInSDKBuildFile(self):
    """Tests that an absolute EDK path in an SDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _SDK_BUILD_FILE,
        [ self.edk_absolute_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    expected_message = PRESUBMIT._ILLEGAL_EXTERNAL_PATH_WARNING_MESSAGE
    self.checkWarningWithSingleItem(warnings[0],
                                    expected_message,
                                    _SDK_BUILD_FILE,
                                    1,
                                    self.edk_absolute_path)

  def testAbsoluteSDKReferenceInEDKBuildFile(self):
    """Tests that an absolute SDK path within an EDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _EDK_BUILD_FILE,
        [ self.sdk_relative_path, self.sdk_absolute_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())
    self.assertEqual(1, len(warnings))
    self.checkSDKAbsolutePathWarningWithSingleItem(warnings[0],
                                                   'EDK',
                                                   _EDK_BUILD_FILE,
                                                   2,
                                                   self.sdk_absolute_path)

  def testAbsoluteEDKReferenceInEDKBuildFile(self):
    """Tests that an absolute EDK path in an EDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _EDK_BUILD_FILE,
        [ self.edk_absolute_path, self.edk_relative_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    expected_message = PRESUBMIT._ILLEGAL_EDK_ABSOLUTE_PATH_WARNING_MESSAGE
    self.checkWarningWithSingleItem(warnings[0],
                                    expected_message,
                                    _EDK_BUILD_FILE,
                                    1,
                                    self.edk_absolute_path)

  def testExternalReferenceInEDKBuildFile(self):
    """Tests that an external path in an EDK buildfile is not flagged."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _EDK_BUILD_FILE,
        [ self.non_whitelisted_external_path, self.whitelisted_external_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(warnings))

  def testIrrelevantBuildFile(self):
    """Tests that nothing is flagged in a non SDK/EDK buildfile."""
    mock_input_api = self.inputApiContainingFileWithPaths(
        _IRRELEVANT_BUILD_FILE,
        [ self.sdk_absolute_path,
          self.sdk_relative_path,
          self.edk_absolute_path,
          self.edk_relative_path,
          self.non_whitelisted_external_path,
          self.whitelisted_external_path ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(warnings))

class SourceSetTypesInBuildFilesTest(unittest.TestCase):
  """Tests checking of correct source set types within SDK/EDK buildfiles."""

  def inputApiContainingFileWithSourceSets(self, filename, source_sets):
    """Returns a MockInputApi object containing a single file having |filename|
    as its name and |source_sets| as its contents."""
    mock_file = MockFile(filename, source_sets)
    mock_input_api = MockInputApi()
    mock_input_api.files.append(mock_file)
    return mock_input_api

  def checkWarningWithSingleItem(self,
                                 warning,
                                 package,
                                 build_file,
                                 line_num):
    """Checks that warning has the expected incorrect source set type message
    for |package| and a single item whose contents are the incorrect source
    set type item for (build_file, line_num)."""
    expected_message = \
        PRESUBMIT._INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGES[package]
    self.assertEqual(expected_message, warning.message)
    self.assertEqual(1, len(warning.items))
    expected_item = PRESUBMIT._IncorrectSourceSetTypeWarningItem(
        build_file, line_num)
    self.assertEqual(expected_item, warning.items[0])

  def testNakedSourceSetInSDKBuildFile(self):
    """Tests that a source_set within an SDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _SDK_BUILD_FILE,
        [ 'mojo_sdk_source_set(', 'source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    self.checkWarningWithSingleItem(warnings[0], 'SDK', _SDK_BUILD_FILE, 2)

  def testPythonSourceSetInSDKBuildFile(self):
    """Tests that a python_binary_source_set within an SDK buildfile is
    accepted."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _SDK_BUILD_FILE,
        [ 'mojo_sdk_source_set(', 'python_binary_source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(0, len(warnings))

  def testEDKSourceSetInSDKBuildFile(self):
    """Tests that a mojo_edk_source_set within an SDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _SDK_BUILD_FILE,
        [ 'mojo_sdk_source_set(', 'mojo_edk_source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    self.checkWarningWithSingleItem(warnings[0], 'SDK', _SDK_BUILD_FILE, 2)

  def testNakedSourceSetInEDKBuildFile(self):
    """Tests that a source_set within an EDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _EDK_BUILD_FILE,
        [ 'source_set(', 'mojo_edk_source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    self.checkWarningWithSingleItem(warnings[0], 'EDK', _EDK_BUILD_FILE, 1)

  def testSDKSourceSetInEDKBuildFile(self):
    """Tests that a mojo_sdk_source_set within an EDK buildfile is flagged."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _EDK_BUILD_FILE,
        [ 'mojo_sdk_source_set(', 'mojo_edk_source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())

    self.assertEqual(1, len(warnings))
    self.checkWarningWithSingleItem(warnings[0], 'EDK', _EDK_BUILD_FILE, 1)

  def testIrrelevantBuildFile(self):
    """Tests that a source_set in a non-SDK/EDK buildfile isn't flagged."""
    mock_input_api = self.inputApiContainingFileWithSourceSets(
        _IRRELEVANT_BUILD_FILE,
        [ 'source_set(' ])
    warnings = PRESUBMIT._BuildFileChecks(mock_input_api, MockOutputApi())
    self.assertEqual(0, len(warnings))

if __name__ == '__main__':
  unittest.main()
