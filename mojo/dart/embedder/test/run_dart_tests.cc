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

static void RunTest(const base::FilePath& path,
                    const char** extra_args,
                    int num_extra_args) {
  // Read in the source.
  std::string source;
  EXPECT_TRUE(ReadFileToString(path, &source)) << "Failed to read test file";

  // Setup the package root.
  base::FilePath package_root;
  PathService::Get(base::DIR_EXE, &package_root);
  package_root = package_root.AppendASCII("gen");

  // All tests run with these arguments.
  const int kNumArgs = num_extra_args + 3;
  const char* args[kNumArgs];
  args[0] = "--enable_asserts";
  args[1] = "--enable_type_checks";
  args[2] = "--error_on_bad_type";

  // Copy the passed in arguments.
  for (int i = 0; i < num_extra_args; ++i) {
    args[i + 3] = extra_args[i];
  }

  char* error = NULL;
  bool success = DartController::runDartScript(
      NULL,  // No app data.
      source,
      path.AsUTF8Unsafe(),
      package_root.AsUTF8Unsafe(),
      NULL,  // No creation callback.
      NULL,  // No shutdown callback.
      generateEntropy,
      args,
      kNumArgs,
      &error);
  EXPECT_TRUE(success) << error;
}

static void RunEmbedderTest(const std::string& test,
                            const char** extra_args,
                            int num_extra_args) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("mojo")
             .AppendASCII("dart")
             .AppendASCII("embedder")
             .AppendASCII("test")
             .AppendASCII(test);
  RunTest(path, extra_args, num_extra_args);
}

static void RunMojoTest(const std::string& test,
                        const char** extra_args,
                        int num_extra_args) {
  base::FilePath path;
  PathService::Get(base::DIR_SOURCE_ROOT, &path);
  path = path.AppendASCII("mojo")
             .AppendASCII("dart")
             .AppendASCII("test")
             .AppendASCII(test);
  RunTest(path, extra_args, num_extra_args);
}

TEST(DartTest, hello_mojo) {
  RunEmbedderTest("hello_mojo.dart", nullptr, 0);
}

TEST(DartTest, core_types_test) {
  RunEmbedderTest("core_types_test.dart", nullptr, 0);
}

TEST(DartTest, async_test) {
  RunEmbedderTest("async_test.dart", nullptr, 0);
}

TEST(DartTest, import_mojo) {
  RunEmbedderTest("import_mojo.dart", nullptr, 0);
}

TEST(DartTest, core_test) {
  RunMojoTest("core_test.dart", nullptr, 0);
}

TEST(DartTest, codec_test) {
  RunMojoTest("codec_test.dart", nullptr, 0);
}

}  // namespace
}  // namespace dart
}  // namespace mojo
