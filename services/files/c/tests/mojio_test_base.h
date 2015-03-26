// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_C_TESTS_MOJIO_TEST_BASE_H_
#define SERVICES_FILES_C_TESTS_MOJIO_TEST_BASE_H_

#include "mojo/public/cpp/environment/logging.h"
#include "services/files/c/lib/singletons.h"
#include "services/files/c/tests/mojio_impl_test_base.h"

namespace mojio {
namespace test {

// This is a base class for tests that test the exposed functions (etc.), and
// which probably use the singletons.
class MojioTestBase : public MojioImplTestBase {
 public:
  MojioTestBase() {}
  ~MojioTestBase() override {}

  void SetUp() override {
    MojioImplTestBase::SetUp();

    // Explicitly set up the singletons.
    MOJO_CHECK(singletons::GetErrnoImpl());
    MOJO_CHECK(singletons::GetFDTable());
    singletons::SetCurrentWorkingDirectory(directory().Pass());
    MOJO_CHECK(singletons::GetCurrentWorkingDirectory());
  }

  void TearDown() override {
    // Explicitly tear down the singletons.
    singletons::ResetCurrentWorkingDirectory();
    singletons::ResetFDTable();
    singletons::ResetErrnoImpl();

    MojioImplTestBase::TearDown();
  }

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(MojioTestBase);
};

}  // namespace test
}  // namespace mojio

#endif  // SERVICES_FILES_C_TESTS_MOJIO_TEST_BASE_H_
