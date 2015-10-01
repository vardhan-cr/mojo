// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_
#define SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/native_support/public/interfaces/process.mojom.h"

namespace mojo {
class ApplicationConnection;
}

namespace native_support {

class ProcessIORedirection;

class ProcessImpl : public Process {
 public:
  ProcessImpl(scoped_refptr<base::TaskRunner> worker_runner,
              mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<Process> request);
  ~ProcessImpl() override;

  // |Process| implementation:
  void Spawn(const mojo::String& path,
             mojo::Array<mojo::String> argv,
             mojo::Array<mojo::String> envp,
             mojo::files::FilePtr stdin_file,
             mojo::files::FilePtr stdout_file,
             mojo::files::FilePtr stderr_file,
             mojo::InterfaceRequest<ProcessController> process_controller,
             const SpawnCallback& callback) override;
  void SpawnWithTerminal(
      const mojo::String& path,
      mojo::Array<mojo::String> argv,
      mojo::Array<mojo::String> envp,
      mojo::files::FilePtr terminal_file,
      mojo::InterfaceRequest<ProcessController> process_controller,
      const SpawnWithTerminalCallback& callback) override;

 private:
  // Note: We take advantage of the fact that |SpawnCallback| and
  // |SpawnWithTerminalCallback| are the same.
  void SpawnImpl(const mojo::String& path,
                 mojo::Array<mojo::String> argv,
                 mojo::Array<mojo::String> envp,
                 std::unique_ptr<ProcessIORedirection> process_io_redirection,
                 const std::vector<int>& fds_to_inherit,
                 mojo::InterfaceRequest<ProcessController> process_controller,
                 const SpawnCallback& callback);

  scoped_refptr<base::TaskRunner> worker_runner_;
  mojo::StrongBinding<Process> binding_;

  DISALLOW_COPY_AND_ASSIGN(ProcessImpl);
};

}  // namespace native_support

#endif  // SERVICES_NATIVE_SUPPORT_PROCESS_IMPL_H_
