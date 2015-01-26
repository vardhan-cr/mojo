// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/external_application_registrar_connection.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/edk/embedder/channel_init.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "shell/domain_socket/net_errors.h"
#include "shell/domain_socket/socket_descriptor.h"
#include "shell/domain_socket/unix_domain_client_socket_posix.h"
#include "shell/external_application_registrar.mojom.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {

ExternalApplicationRegistrarConnection::ExternalApplicationRegistrarConnection(
    const base::FilePath& socket_path)
    : client_socket_(new UnixDomainClientSocket(socket_path.value(), false)) {
}

ExternalApplicationRegistrarConnection::
    ~ExternalApplicationRegistrarConnection() {
  channel_init_.WillDestroySoon();
}

void ExternalApplicationRegistrarConnection::OnConnectionError() {
  channel_init_.WillDestroySoon();
}

bool ExternalApplicationRegistrarConnection::Connect() {
  DCHECK(client_socket_) << "Single use only.";
  base::RunLoop loop;
  int rv = client_socket_->Connect(
      base::Bind(&ExternalApplicationRegistrarConnection::OnConnect,
                 base::Unretained(this), loop.QuitClosure()));
  if (rv != net::ERR_IO_PENDING) {
    DVLOG(1) << "Connect returning immediately: " << net::ErrorToString(rv);
    OnConnect(base::Bind(&base::DoNothing), rv);
    return registrar_;
  }
  DVLOG(1) << "Waiting for connection.";
  loop.Run();
  return registrar_;
}

void ExternalApplicationRegistrarConnection::Register(
    const GURL& app_url,
    const std::vector<std::string>& args,
    base::Callback<void(ShellPtr)> register_complete_callback) {
  DCHECK(!client_socket_);
  registrar_->Register(String::From(app_url), Array<String>::From(args),
                       register_complete_callback);
}

void ExternalApplicationRegistrarConnection::OnConnect(const base::Closure& cb,
                                                       int rv) {
  if (rv != net::OK) {
    LOG(ERROR) << "OnConnect called with error: " << net::ErrorToString(rv);
    cb.Run();
    return;
  }

  mojo::ScopedMessagePipeHandle ptr_message_pipe_handle =
      channel_init_.Init(client_socket_->ReleaseConnectedSocket(),
                         base::MessageLoopProxy::current());
  CHECK(ptr_message_pipe_handle.is_valid());
  client_socket_.reset();  // This is dead now, ensure it can't be reused.

  registrar_.Bind(ptr_message_pipe_handle.Pass());
  cb.Run();
}

}  // namespace shell
}  // namespace mojo
