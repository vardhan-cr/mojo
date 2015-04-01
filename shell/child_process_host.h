// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CHILD_PROCESS_HOST_H_
#define SHELL_CHILD_PROCESS_HOST_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/process/process.h"
#include "mojo/edk/embedder/channel_info_forward.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "shell/child_controller.mojom.h"

namespace mojo {
namespace shell {

class Context;

// Child process host: parent-process representation of a child process, which
// hosts/runs a native Mojo application loaded from the file system. This class
// handles launching and communicating with the child process.
//
// This class is not thread-safe. It should be created/used/destroyed on a
// single thread.
class ChildProcessHost : public ErrorHandler {
 public:
  explicit ChildProcessHost(Context* context);
  virtual ~ChildProcessHost();

  // |Start()|s the child process; calls |DidStart()| (on the thread on which
  // |Start()| was called) when the child has been started (or failed to start).
  // After calling |Start()|, this object must not be destroyed until
  // |DidStart()| has been called.
  // TODO(vtl): Consider using weak pointers and removing this requirement.
  // TODO(vtl): This should probably take a callback instead.
  // TODO(vtl): Consider merging this with |StartApp()|.
  void Start();

  // Waits for the child process to terminate, and returns its exit code.
  // Note: If |Start()| has been called, this must not be called until the
  // callback has been called.
  int Join();

  // Methods relayed to the |ChildController|. These methods may be only be
  // called after |Start()|, but may be called immediately (without waiting for
  // |DidStart()|).

  // Like |ChildController::StartApp()|, but with one difference:
  // |on_app_complete| will *always* get called, even on connection error (or
  // even if the child process failed to start at all).
  void StartApp(const String& app_path,
                bool clean_app_path,
                InterfaceRequest<Application> application_request,
                const ChildController::StartAppCallback& on_app_complete);
  void ExitNow(int32_t exit_code);

  // TODO(vtl): This is virtual, so tests can override it, but really |Start()|
  // should take a callback (see above) and this should be private.
  virtual void DidStart(bool success);

 private:
  // Callback for |embedder::CreateChannel()|.
  void DidCreateChannel(embedder::ChannelInfo* channel_info);

  bool DoLaunch();

  void AppCompleted(int32_t result);

  // |ErrorHandler| methods:
  void OnConnectionError() override;

  Context* const context_;
  embedder::PlatformChannelPair platform_channel_pair_;

  ChildControllerPtr controller_;
  embedder::ChannelInfo* channel_info_;
  ChildController::StartAppCallback on_app_complete_;

  base::Process child_process_;

  DISALLOW_COPY_AND_ASSIGN(ChildProcessHost);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_CHILD_PROCESS_HOST_H_
