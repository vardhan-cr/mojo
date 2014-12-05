// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_EXTERNAL_APPLICATION_LISTENER_WIN_H_
#define SHELL_EXTERNAL_APPLICATION_LISTENER_WIN_H_

#include "shell/external_application_listener.h"

#include "base/callback.h"
#include "base/files/file_path.h"

namespace mojo {
namespace shell {

// External application registration is not supported on Windows, hence this
// stub.
class ExternalApplicationListenerStub : public ExternalApplicationListener {
 public:
  ExternalApplicationListenerStub();
  virtual ~ExternalApplicationListenerStub() override;

  void ListenInBackground(const base::FilePath& listen_socket_path,
                          const RegisterCallback& register_callback) override;
  void ListenInBackgroundWithErrorCallback(
      const base::FilePath& listen_socket_path,
      const RegisterCallback& register_callback,
      const ErrorCallback& error_callback) override;
  void WaitForListening() override;
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_EXTERNAL_APPLICATION_LISTENER_WIN_H_
