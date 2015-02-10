// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/shell_test_base.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "shell/filename_util.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {
namespace test {

ShellTestBase::ShellTestBase() {
}

ShellTestBase::~ShellTestBase() {
}

void ShellTestBase::SetUp() {
  shell_context_.Init();
  SetUpTestApplications();
}

ScopedMessagePipeHandle ShellTestBase::ConnectToService(
    const GURL& application_url,
    const std::string& service_name) {
  return shell_context_.ConnectToServiceByName(application_url, service_name)
      .Pass();
}

#if !defined(OS_ANDROID)
void ShellTestBase::SetUpTestApplications() {
  // Set the URLResolver origin to be the same as the base file path for
  // local files. This is primarily for test convenience, so that references
  // to unknown mojo: urls that do not have specific local file or custom
  // mappings registered on the URL resolver are treated as shared libraries.
  base::FilePath service_dir;
  CHECK(PathService::Get(base::DIR_MODULE, &service_dir));
  shell_context_.url_resolver()->SetMojoBaseURL(FilePathToFileURL(service_dir));
}
#endif

}  // namespace test
}  // namespace shell
}  // namespace mojo
