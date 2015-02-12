// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_EXTERNAL_APPLICATION_REGISTRAR_CONNECTION_H_
#define SHELL_EXTERNAL_APPLICATION_REGISTRAR_CONNECTION_H_

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/edk/embedder/channel_info_forward.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "shell/domain_socket/socket_descriptor.h"
#include "shell/domain_socket/unix_domain_client_socket_posix.h"
#include "shell/external_application_registrar.mojom.h"
#include "url/gurl.h"

namespace mojo {

class Application;

namespace shell {

// Externally-running applications can use this class to discover and register
// with a running mojo_shell instance.
// MUST be run on an IO thread
class ExternalApplicationRegistrarConnection : public ErrorHandler {
 public:
  // Configures client_socket_ to point at socket_path.
  explicit ExternalApplicationRegistrarConnection(
      const base::FilePath& socket_path);
  ~ExternalApplicationRegistrarConnection() override;

  // Implementation of ErrorHandler
  void OnConnectionError() override;

  // Connects the client socket and spins a message loop until the connection is
  // initiated, returning false if it was unable to do so.
  bool Connect();

  using RegisterCallback = base::Callback<void(InterfaceRequest<Application>)>;

  // Registers this app with the shell at the provided URL.
  void Register(const GURL& app_url,
                const std::vector<std::string>& args,
                const RegisterCallback& callback);

 private:
  // Handles the result of Connect(). If it was successful, promotes the socket
  // to a MessagePipe and binds it to registrar_.
  void OnConnect(const base::Closure& cb, int rv);

  scoped_ptr<UnixDomainClientSocket> client_socket_;
  embedder::ChannelInfo* channel_info_;
  ExternalApplicationRegistrarPtr registrar_;
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_EXTERNAL_APPLICATION_REGISTRAR_CONNECTION_H_
