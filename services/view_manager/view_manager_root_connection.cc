// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/view_manager_root_connection.h"

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/view_manager/client_connection.h"
#include "services/view_manager/connection_manager.h"
#include "services/view_manager/display_manager.h"
#include "services/view_manager/view_manager_service_impl.h"

using mojo::ApplicationConnection;
using mojo::InterfaceRequest;
using mojo::ViewManagerService;
using mojo::WindowManagerInternalClient;

namespace view_manager {

ViewManagerRootConnection::ViewManagerRootConnection(
    mojo::ApplicationImpl* application_impl,
    ViewManagerRootConnectionObserver* observer)
    : app_impl_(application_impl), observer_(observer) {}

ViewManagerRootConnection::~ViewManagerRootConnection() {
  observer_->OnCloseViewManagerRootConnection(this);
}

bool ViewManagerRootConnection::Init(mojo::ApplicationConnection* connection) {
  if (connection_manager_.get()) {
    NOTREACHED() << "Only one incoming connection is allowed.";
  }
  // |connection| originates from the WindowManager. Let it connect directly
  // to the ViewManager and WindowManagerInternalClient.
  connection->AddService<ViewManagerService>(this);
  connection->AddService<WindowManagerInternalClient>(this);
  connection->ConnectToService(&wm_internal_);
  // If no ServiceProvider has been sent, refuse the connection.
  if (!wm_internal_) {
    delete this;
    return false;
  }
  wm_internal_.set_connection_error_handler(
      base::Bind(&ViewManagerRootConnection::OnLostConnectionToWindowManager,
                 base::Unretained(this)));

  scoped_ptr<DefaultDisplayManager> display_manager(new DefaultDisplayManager(
      app_impl_, connection,
      base::Bind(&ViewManagerRootConnection::OnLostConnectionToWindowManager,
                 base::Unretained(this))));
  connection_manager_.reset(
      new ConnectionManager(this, display_manager.Pass(), wm_internal_.get()));
  return true;
}

void ViewManagerRootConnection::OnLostConnectionToWindowManager() {
  delete this;
}

ClientConnection*
ViewManagerRootConnection::CreateClientConnectionForEmbedAtView(
    ConnectionManager* connection_manager,
    mojo::InterfaceRequest<mojo::ViewManagerService> service_request,
    mojo::ConnectionSpecificId creator_id,
    const std::string& creator_url,
    const std::string& url,
    const ViewId& root_id) {
  mojo::ViewManagerClientPtr client;
  app_impl_->ConnectToService(url, &client);

  scoped_ptr<ViewManagerServiceImpl> service(new ViewManagerServiceImpl(
      connection_manager, creator_id, creator_url, url, root_id));
  return new DefaultClientConnection(service.Pass(), connection_manager,
                                     service_request.Pass(), client.Pass());
}

ClientConnection*
ViewManagerRootConnection::CreateClientConnectionForEmbedAtView(
    ConnectionManager* connection_manager,
    mojo::InterfaceRequest<mojo::ViewManagerService> service_request,
    mojo::ConnectionSpecificId creator_id,
    const std::string& creator_url,
    const ViewId& root_id,
    mojo::ViewManagerClientPtr view_manager_client) {
  scoped_ptr<ViewManagerServiceImpl> service(new ViewManagerServiceImpl(
      connection_manager, creator_id, creator_url, std::string(), root_id));
  return new DefaultClientConnection(service.Pass(), connection_manager,
                                     service_request.Pass(),
                                     view_manager_client.Pass());
}

void ViewManagerRootConnection::Create(
    ApplicationConnection* connection,
    InterfaceRequest<ViewManagerService> request) {
  if (connection_manager_->has_window_manager_client_connection()) {
    VLOG(1) << "ViewManager interface requested more than once.";
    return;
  }

  scoped_ptr<ViewManagerServiceImpl> service(new ViewManagerServiceImpl(
      connection_manager_.get(), kInvalidConnectionId, std::string(),
      std::string("mojo:window_manager"), RootViewId()));
  mojo::ViewManagerClientPtr client;
  wm_internal_client_request_ = GetProxy(&client);
  scoped_ptr<ClientConnection> client_connection(
      new DefaultClientConnection(service.Pass(), connection_manager_.get(),
                                  request.Pass(), client.Pass()));
  connection_manager_->SetWindowManagerClientConnection(
      client_connection.Pass());
}

void ViewManagerRootConnection::Create(
    ApplicationConnection* connection,
    InterfaceRequest<WindowManagerInternalClient> request) {
  if (wm_internal_client_binding_.get()) {
    VLOG(1) << "WindowManagerInternalClient requested more than once.";
    return;
  }

  // ConfigureIncomingConnection() must have been called before getting here.
  DCHECK(connection_manager_.get());
  wm_internal_client_binding_.reset(
      new mojo::Binding<WindowManagerInternalClient>(connection_manager_.get(),
                                                     request.Pass()));
  wm_internal_client_binding_->set_connection_error_handler(
      base::Bind(&ViewManagerRootConnection::OnLostConnectionToWindowManager,
                 base::Unretained(this)));
  wm_internal_->SetViewManagerClient(
      wm_internal_client_request_.PassMessagePipe());
}

}  // namespace view_manager
