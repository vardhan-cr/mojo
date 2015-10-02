// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/files/files_test_base.h"

#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/files/public/interfaces/directory.mojom.h"
#include "mojo/services/files/public/interfaces/types.mojom.h"

namespace mojo {
namespace files {

FilesTestBase::FilesTestBase() {
}

FilesTestBase::~FilesTestBase() {
}

void FilesTestBase::SetUp() {
  test::ApplicationTestBase::SetUp();
  application_impl()->ConnectToService("mojo:files", &files_);
}

void FilesTestBase::GetTemporaryRoot(DirectoryPtr* directory) {
  Error error = Error::INTERNAL;
  files()->OpenFileSystem(nullptr, GetProxy(directory), Capture(&error));
  ASSERT_TRUE(files().WaitForIncomingResponse());
  ASSERT_EQ(Error::OK, error);
}

}  // namespace files
}  // namespace mojo
