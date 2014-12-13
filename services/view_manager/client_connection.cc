// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/client_connection.h"

#include "services/view_manager/connection_manager.h"
#include "services/view_manager/view_manager_service_impl.h"

namespace view_manager {

ClientConnection::ClientConnection(scoped_ptr<ViewManagerServiceImpl> service)
    : service_(service.Pass()), client_(nullptr) {
}

ClientConnection::~ClientConnection() {
}

DefaultClientConnection::DefaultClientConnection(
    scoped_ptr<ViewManagerServiceImpl> service_impl,
    ConnectionManager* connection_manager,
    mojo::ScopedMessagePipeHandle handle)
    : ClientConnection(service_impl.Pass()),
      connection_manager_(connection_manager),
      binding_(service(), handle.Pass()) {
  binding_.set_error_handler(this);
  set_client(binding_.client());
}

DefaultClientConnection::~DefaultClientConnection() {
}

void DefaultClientConnection::OnConnectionError() {
  connection_manager_->OnConnectionError(this);
}

}  // namespace view_manager
