// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_DYNAMIC_SERVICE_RUNNER_H_
#define SHELL_DYNAMIC_SERVICE_RUNNER_H_

#include "base/callback_forward.h"
#include "base/memory/scoped_ptr.h"
#include "base/native_library.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/system/core.h"
#include "shell/application_manager/application_manager.h"

namespace base {
class FilePath;
}

namespace mojo {
class Application;

namespace shell {

class Context;

// Loads the service in the DSO specificed by |app_path| and prepares it for
// execution. Runs the DSO's exported function MojoMain().
// The NativeLibrary is returned and ownership transferred to the caller.
// This is so if it is unloaded at all, this can be done safely after this
// thread is destroyed and any thread-local destructors have been executed.
base::NativeLibrary LoadAndRunNativeApplication(
    const base::FilePath& app_path,
    mojo::NativeRunner::CleanupBehavior cleanup_behavior,
    InterfaceRequest<Application> application_request);

// A generic factory.
template <class DynamicServiceRunnerImpl>
class DynamicServiceRunnerFactoryImpl : public mojo::NativeRunnerFactory {
 public:
  DynamicServiceRunnerFactoryImpl(shell::Context* context)
      : context_(context) {}
  virtual ~DynamicServiceRunnerFactoryImpl() {}
  scoped_ptr<NativeRunner> Create() override {
    return scoped_ptr<NativeRunner>(new DynamicServiceRunnerImpl(context_));
  }

 private:
  shell::Context* context_;
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_DYNAMIC_SERVICE_RUNNER_H_
