// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_CLIENT_CONNECTION_H_
#define SERVICES_VIEW_MANAGER_CLIENT_CONNECTION_H_

#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/services/view_manager/public/interfaces/view_manager.mojom.h"

namespace view_manager {

class ConnectionManager;
class ViewManagerServiceImpl;

// ClientConnection encapsulates the state needed for a single client connected
// to the view manager.
class ClientConnection {
 public:
  explicit ClientConnection(scoped_ptr<ViewManagerServiceImpl> service);
  virtual ~ClientConnection();

  ViewManagerServiceImpl* service() { return service_.get(); }
  const ViewManagerServiceImpl* service() const { return service_.get(); }

  mojo::ViewManagerClient* client() { return client_; }

 protected:
  void set_client(mojo::ViewManagerClient* client) { client_ = client; }

 private:
  scoped_ptr<ViewManagerServiceImpl> service_;
  mojo::ViewManagerClient* client_;

  DISALLOW_COPY_AND_ASSIGN(ClientConnection);
};

// Bindings implementation of ClientConnection.
class DefaultClientConnection : public ClientConnection,
                                public mojo::ErrorHandler {
 public:
  DefaultClientConnection(scoped_ptr<ViewManagerServiceImpl> service_impl,
                          ConnectionManager* connection_manager,
                          mojo::ScopedMessagePipeHandle handle);
  ~DefaultClientConnection() override;

 private:
  // ErrorHandler:
  void OnConnectionError() override;

  ConnectionManager* connection_manager_;
  mojo::Binding<mojo::ViewManagerService> binding_;

  DISALLOW_COPY_AND_ASSIGN(DefaultClientConnection);
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_CLIENT_CONNECTION_H_
