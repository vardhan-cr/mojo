# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Presubmit script for mojo

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details about the presubmit API built into depot_tools.
"""

import os.path
import re

_SDK_WHITELISTED_EXTERNAL_PATHS = [
  "//testing/gtest",
  "//third_party/cython",
  "//third_party/khronos",
]

_PACKAGE_PATH_PREFIXES = {"SDK": "mojo/public/",
                          "EDK": "mojo/edk/"}

# TODO(etiennej): python_binary_source_set added due to crbug.com/443147
_PACKAGE_SOURCE_SET_TYPES = {"SDK": ["mojo_sdk_source_set",
                                     "python_binary_source_set"],
                             "EDK": ["mojo_edk_source_set"]}

_ILLEGAL_EXTERNAL_PATH_WARNING_MESSAGE = \
    "Found disallowed external paths within SDK buildfiles."

_ILLEGAL_EDK_ABSOLUTE_PATH_WARNING_MESSAGE = \
    "Found references to the EDK via absolute paths within EDK buildfiles.",

_ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGE_TEMPLATE = \
    "Found references to the SDK via absolute paths within %s buildfiles."

_ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGES = {
  "SDK": _ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGE_TEMPLATE % "SDK",
  "EDK": _ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGE_TEMPLATE % "EDK",
}

_INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGE_TEMPLATE = \
    "All source sets in the %s must be constructed via %s."

_INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGES = {
  "SDK": _INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGE_TEMPLATE
      % ("SDK", _PACKAGE_SOURCE_SET_TYPES["SDK"]),
  "EDK": _INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGE_TEMPLATE
      % ("EDK", _PACKAGE_SOURCE_SET_TYPES["EDK"]),
}

def _IsBuildFileWithinPackage(f, package):
  """Returns whether |f| specifies a GN build file within |package| ("SDK" or
  "EDK")."""
  assert package in _PACKAGE_PATH_PREFIXES
  package_path_prefix = _PACKAGE_PATH_PREFIXES[package]

  if not f.LocalPath().startswith(package_path_prefix):
    return False
  if (not f.LocalPath().endswith("/BUILD.gn") and
      not f.LocalPath().endswith(".gni")):
    return False
  return True

def _AffectedBuildFilesWithinPackage(input_api, package):
  """Returns all the affected build files within |package| ("SDK" or "EDK")."""
  return [f for f in input_api.AffectedFiles()
      if _IsBuildFileWithinPackage(f, package)]

def _FindIllegalAbsolutePathsInBuildFiles(input_api, package):
  """Finds illegal absolute paths within the build files in
  |input_api.AffectedFiles()| that are within |package| ("SDK" or "EDK").
  An illegal absolute path within the SDK is one that is to the SDK itself
  or a non-whitelisted external path. An illegal absolute path within the
  EDK is one that is to the SDK or the EDK.
  Returns any such references in a list of (file_path, line_number,
  referenced_path) tuples."""
  illegal_references = []
  for f in _AffectedBuildFilesWithinPackage(input_api, package):
    for line_num, line in f.ChangedContents():
      # Determine if this is a reference to an absolute path.
      m = re.search(r'"(//[^"]*)"', line)
      if not m:
        continue
      referenced_path = m.group(1)

      # In the EDK, all external absolute paths are allowed.
      if package == "EDK" and not referenced_path.startswith("//mojo"):
        continue

      # Determine if this is a whitelisted external path.
      if referenced_path in _SDK_WHITELISTED_EXTERNAL_PATHS:
        continue

      illegal_references.append((f.LocalPath(), line_num, referenced_path))

  return illegal_references

def _PathReferenceInBuildFileWarningItem(build_file, line_num, referenced_path):
  """Returns a string expressing a warning item that |referenced_path| is
  referenced at |line_num| in |build_file|."""
  return "%s, line %d (%s)" % (build_file, line_num, referenced_path)

def _IncorrectSourceSetTypeWarningItem(build_file, line_num):
  """Returns a string expressing that the error occurs at |line_num| in
  |build_file|."""
  return "%s, line %d" % (build_file, line_num)

def _CheckNoIllegalAbsolutePathsInBuildFiles(input_api, output_api, package):
  """Makes sure that the BUILD.gn files within |package| ("SDK" or "EDK") do not
  reference the SDK/EDK via absolute paths, and do not reference disallowed
  external dependencies."""
  sdk_references = []
  edk_references = []
  external_deps_references = []

  # Categorize any illegal references.
  illegal_references = _FindIllegalAbsolutePathsInBuildFiles(input_api, package)
  for build_file, line_num, referenced_path in illegal_references:
    reference_string = _PathReferenceInBuildFileWarningItem(build_file,
                                                            line_num,
                                                            referenced_path)
    if referenced_path.startswith("//mojo/public"):
      sdk_references.append(reference_string)
    elif package == "SDK":
      external_deps_references.append(reference_string)
    elif referenced_path.startswith("//mojo/edk"):
      edk_references.append(reference_string)

  # Package up categorized illegal references into results.
  results = []
  if sdk_references:
    results.extend([output_api.PresubmitError(
        _ILLEGAL_SDK_ABSOLUTE_PATH_WARNING_MESSAGES[package],
        items=sdk_references)])

  if external_deps_references:
    assert package == "SDK"
    results.extend([output_api.PresubmitError(
        _ILLEGAL_EXTERNAL_PATH_WARNING_MESSAGE,
        items=external_deps_references)])

  if edk_references:
    assert package == "EDK"
    results.extend([output_api.PresubmitError(
        _ILLEGAL_EDK_ABSOLUTE_PATH_WARNING_MESSAGE,
        items=edk_references)])

  return results

def _CheckSourceSetsAreOfCorrectType(input_api, output_api, package):
  """Makes sure that the BUILD.gn files always use the correct wrapper type for
  |package|, which can be one of ["SDK", "EDK"], to construct source_set
  targets."""
  assert package in _PACKAGE_SOURCE_SET_TYPES
  required_source_set_type = _PACKAGE_SOURCE_SET_TYPES[package]

  problems = []
  for f in _AffectedBuildFilesWithinPackage(input_api, package):
    for line_num, line in f.ChangedContents():
      m = re.search(r"[a-z_]*source_set\(", line)
      if not m:
        continue
      source_set_type = m.group(0)[:-1]
      if source_set_type in required_source_set_type:
        continue
      problems.append(_IncorrectSourceSetTypeWarningItem(f.LocalPath(),
                                                         line_num))

  if not problems:
    return []
  return [output_api.PresubmitError(
      _INCORRECT_SOURCE_SET_TYPE_WARNING_MESSAGES[package],
      items=problems)]

def _CheckChangePylintsClean(input_api, output_api):
  # Additional python module paths (we're in src/mojo/); not everyone needs
  # them, but it's easiest to add them to everyone's path.
  # For ply and jinja2:
  third_party_path = os.path.join(
      input_api.PresubmitLocalPath(), "..", "third_party")
  # For the bindings generator:
  mojo_public_bindings_pylib_path = os.path.join(
      input_api.PresubmitLocalPath(), "public", "tools", "bindings", "pylib")
  # For the python bindings:
  mojo_python_bindings_path = os.path.join(
      input_api.PresubmitLocalPath(), "public", "python")
  # For the python bindings tests:
  mojo_python_bindings_tests_path = os.path.join(
      input_api.PresubmitLocalPath(), "python", "tests")
  # For the roll tools scripts:
  mojo_roll_tools_path = os.path.join(
      input_api.PresubmitLocalPath(), "tools", "roll")
  # For all mojo/tools scripts
  mopy_path = os.path.join(input_api.PresubmitLocalPath(), "tools")
  # TODO(vtl): Don't lint these files until the (many) problems are fixed
  # (possibly by deleting/rewriting some files).
  temporary_black_list = input_api.DEFAULT_BLACK_LIST + \
      (r".*\bpublic[\\\/]tools[\\\/]bindings[\\\/]pylib[\\\/]mojom[\\\/]"
           r"generate[\\\/].+\.py$",
       r".*\bpublic[\\\/]tools[\\\/]bindings[\\\/]generators[\\\/].+\.py$",
       r".*\bspy[\\\/]ui[\\\/].+\.py$")

  results = []
  pylint_extra_paths = [
      third_party_path,
      mojo_public_bindings_pylib_path,
      mojo_python_bindings_path,
      mojo_python_bindings_tests_path,
      mojo_roll_tools_path,
      mopy_path,
  ]
  results.extend(input_api.canned_checks.RunPylint(
      input_api, output_api, extra_paths_list=pylint_extra_paths,
      black_list=temporary_black_list))
  return results

def _BuildFileChecks(input_api, output_api):
  """Performs checks on SDK and EDK buildfiles."""
  results = []
  for package in ["SDK", "EDK"]:
    results.extend(_CheckNoIllegalAbsolutePathsInBuildFiles(input_api,
                                                            output_api,
                                                            package))
    results.extend(_CheckSourceSetsAreOfCorrectType(input_api,
                                                    output_api,
                                                    package))
  return results

def _CommonChecks(input_api, output_api):
  """Checks common to both upload and commit."""
  results = []
  results.extend(_BuildFileChecks(input_api, output_api))
  return results

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  results.extend(_CheckChangePylintsClean(input_api, output_api))
  return results

def CheckChangeOnCommit(input_api, output_api):
  results = []
  results.extend(_CommonChecks(input_api, output_api))
  return results
