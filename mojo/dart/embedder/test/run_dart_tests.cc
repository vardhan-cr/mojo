// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "crypto/random.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/public/cpp/environment/environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace dart {
namespace {

static bool generateEntropy(uint8_t* buffer, intptr_t length) {
  crypto::RandBytes(reinterpret_cast<void*>(buffer), length);
  return true;
}


void RunTest(std::string test) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("mojo")
             .AppendASCII("dart")
             .AppendASCII("embedder")
             .AppendASCII("test")
             .AppendASCII(test);

  // Read in the source.
  std::string source;
  EXPECT_TRUE(ReadFileToString(path, &source)) << "Failed to read test file";

  // Setup the package root.
  base::FilePath package_root;
  PathService::Get(base::DIR_EXE, &package_root);
  package_root = package_root.AppendASCII("gen");

  char* error = NULL;
  bool success = DartController::runDartScript(
      NULL,  // No app data.
      source,
      path.AsUTF8Unsafe(),
      package_root.AsUTF8Unsafe(),
      NULL,  // No creation callback.
      NULL,  // No shutdown callback.
      generateEntropy,
      &error);
  EXPECT_TRUE(success) << error;
}


TEST(DartTest, hello_mojo) {
  RunTest("hello_mojo.dart");
}

}  // namespace
}  // namespace dart
}  // namespace mojo
