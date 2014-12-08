// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/js_app_runner_delegate.h"

#include "base/path_service.h"
#include "gin/modules/console.h"
#include "mojo/edk/js/core.h"
#include "mojo/edk/js/handle.h"
#include "mojo/edk/js/support.h"
#include "mojo/edk/js/threading.h"

namespace js {

namespace {

std::vector<base::FilePath> GetModuleSearchPaths() {
  std::vector<base::FilePath> search_paths(2);
  PathService::Get(base::DIR_SOURCE_ROOT, &search_paths[0]);
  PathService::Get(base::DIR_EXE, &search_paths[1]);
  search_paths[1] = search_paths[1].AppendASCII("gen");
  return search_paths;
}

} // namespace

JSAppRunnerDelegate::JSAppRunnerDelegate()
    : ModuleRunnerDelegate(GetModuleSearchPaths()) {
  AddBuiltinModule(gin::Console::kModuleName, gin::Console::GetModule);
  AddBuiltinModule(mojo::js::Core::kModuleName, mojo::js::Core::GetModule);
  AddBuiltinModule(mojo::js::Support::kModuleName,
                   mojo::js::Support::GetModule);
  AddBuiltinModule(mojo::js::Threading::kModuleName,
                   mojo::js::Threading::GetModule);
}

JSAppRunnerDelegate::~JSAppRunnerDelegate() {
}

void JSAppRunnerDelegate::UnhandledException(gin::ShellRunner* runner,
                                             gin::TryCatch& try_catch) {
  gin::ModuleRunnerDelegate::UnhandledException(runner, try_catch);
  LOG(ERROR) << try_catch.GetStackTrace();
}

} // namespace js

