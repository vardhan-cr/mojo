// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/dart_app.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/dart/embedder/mojo_dart_state.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "third_party/zlib/google/zip_reader.h"
#include "url/gurl.h"

using mojo::Application;

namespace dart {

DartApp::DartApp(mojo::InterfaceRequest<Application> application_request,
                 const base::FilePath& application_dir,
                 bool strict)
    : application_request_(application_request.Pass()),
      application_dir_(application_dir) {
  base::FilePath package_root = application_dir_.AppendASCII("packages");
  base::FilePath entry_path = application_dir_.Append("lib/main.dart");

  config_.application_data = reinterpret_cast<void*>(this);
  config_.strict_compilation = strict;
  config_.script_uri = entry_path.AsUTF8Unsafe();
  config_.package_root = package_root.AsUTF8Unsafe();
  config_.SetVmFlags(nullptr, 0);

  base::MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&DartApp::OnAppLoaded, base::Unretained(this)));
}

// Assume that |url| ends in a file name in lib/, and as a peer to lib/
// is the packages directory.
// 1) Strip the filename.
// 2) Strip lib/
// 3) Append with packages/.
// 4) Reconstruct full url.
static std::string SimplePackageRootFromUrl(std::string url) {
  GURL gurl = GURL(url);
  base::FilePath path = base::FilePath(gurl.path());
  path = path.DirName();
  path = path.DirName();
  path = path.Append("packages");
  path = path.AsEndingWithSeparator();
  const std::string& path_string = path.value();
  GURL::Replacements path_replacement;
  path_replacement.SetPath(path_string.data(),
                           url::Component(0, path_string.size()));
  gurl = gurl.ReplaceComponents(path_replacement);
  return gurl.spec();
}

// Returns the path component from |url|.
static std::string ExtractPath(std::string url) {
  GURL gurl = GURL(url);
  return gurl.path();
}

static bool IsFileScheme(std::string url) {
  GURL gurl = GURL(url);
  return gurl.SchemeIsFile();
}

DartApp::DartApp(mojo::InterfaceRequest<Application> application_request,
                 const std::string& url,
                 bool strict)
    : application_request_(application_request.Pass()) {
  config_.application_data = reinterpret_cast<void*>(this);
  config_.strict_compilation = strict;
  if (IsFileScheme(url)) {
    // Strip file:// and use the path directly.
    config_.script_uri = ExtractPath(url);
    config_.package_root = ExtractPath(SimplePackageRootFromUrl(url));
    config_.use_network_loader = false;
  } else {
    // Use the full url.
    config_.script_uri = url;
    config_.package_root = SimplePackageRootFromUrl(url);
    config_.use_network_loader = true;
  }
  config_.SetVmFlags(nullptr, 0);

  base::MessageLoop::current()->PostTask(FROM_HERE,
      base::Bind(&DartApp::OnAppLoaded, base::Unretained(this)));
}

DartApp::~DartApp() {
}

void DartApp::OnAppLoaded() {
  char* error = nullptr;
  config_.handle = application_request_.PassMessagePipe().release().value();
  config_.error = &error;
  bool success = mojo::dart::DartController::RunDartScript(config_);
  if (!success) {
    LOG(ERROR) << error;
    free(error);
  }
  base::MessageLoop::current()->QuitWhenIdle();
}

}  // namespace dart
