// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILES_C_TESTS_MOJIO_IMPL_TEST_BASE_H_
#define SERVICES_FILES_C_TESTS_MOJIO_IMPL_TEST_BASE_H_

#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/files/public/interfaces/directory.mojom.h"
#include "mojo/services/files/public/interfaces/files.mojom.h"
#include "services/files/c/lib/template_util.h"

namespace mojio {
namespace test {

// This is a base class for tests that test the underlying implementation (i.e.,
// doesn't use the singletons).
class MojioImplTestBase : public mojo::test::ApplicationTestBase {
 public:
  MojioImplTestBase() {}
  ~MojioImplTestBase() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();

    mojo::files::FilesPtr files;
    application_impl()->ConnectToService("mojo:files", &files);

    mojo::files::Error error = mojo::files::ERROR_INTERNAL;
    files->OpenFileSystem(mojo::files::FILE_SYSTEM_TEMPORARY,
                          mojo::GetProxy(&directory_), Capture(&error));
    MOJO_CHECK(files.WaitForIncomingMethodCall());
    MOJO_CHECK(error == mojo::files::ERROR_OK);
  }

 protected:
  mojo::files::DirectoryPtr& directory() { return directory_; }

 private:
  mojo::files::DirectoryPtr directory_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(MojioImplTestBase);
};

}  // namespace test
}  // namespace mojio

#endif  // SERVICES_FILES_C_TESTS_MOJIO_IMPL_TEST_BASE_H_
