// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_VIEW_MANAGER_ROOT_CONNECTION_H_
#define SERVICES_VIEW_MANAGER_VIEW_MANAGER_ROOT_CONNECTION_H_

#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/view_manager/public/interfaces/view_manager.mojom.h"
#include "mojo/services/window_manager/public/interfaces/window_manager_internal.mojom.h"
#include "services/view_manager/connection_manager.h"
#include "services/view_manager/connection_manager_delegate.h"

namespace mojo {
class ApplicationImpl;
}

namespace view_manager {

// An instance of ViewManagerRootConnection represents one inbound connection
// from a window manager to the view manager app. It handles the creation of
// services for this connection, as well as any outbound connection necessary.
class ViewManagerRootConnection
    : public ConnectionManagerDelegate,
      public mojo::InterfaceFactory<mojo::ViewManagerService>,
      public mojo::InterfaceFactory<mojo::WindowManagerInternalClient> {
 public:
  class ViewManagerRootConnectionObserver {
   public:
    // Called when a ViewManagerRootConnection is closing.
    virtual void OnCloseViewManagerRootConnection(
        ViewManagerRootConnection* view_manager_root_connection) = 0;

   protected:
    virtual ~ViewManagerRootConnectionObserver() {}
  };

  ViewManagerRootConnection(mojo::ApplicationImpl* application_impl,
                            ViewManagerRootConnectionObserver* observer);
  ~ViewManagerRootConnection() override;

  // Returns true if the view manager root connection is established, false
  // otherwise. In that case, the object should probably be destroyed.
  bool Init(mojo::ApplicationConnection* connection);

 private:
  // ConnectionManagerDelegate:
  void OnLostConnectionToWindowManager() override;
  ClientConnection* CreateClientConnectionForEmbedAtView(
      ConnectionManager* connection_manager,
      mojo::InterfaceRequest<mojo::ViewManagerService> service_request,
      mojo::ConnectionSpecificId creator_id,
      const std::string& creator_url,
      const std::string& url,
      const ViewId& root_id) override;
  ClientConnection* CreateClientConnectionForEmbedAtView(
      ConnectionManager* connection_manager,
      mojo::InterfaceRequest<mojo::ViewManagerService> service_request,
      mojo::ConnectionSpecificId creator_id,
      const std::string& creator_url,
      const ViewId& root_id,
      mojo::ViewManagerClientPtr view_manager_client) override;

  // mojo::InterfaceFactory<mojo::ViewManagerService>:
  void Create(
      mojo::ApplicationConnection* connection,
      mojo::InterfaceRequest<mojo::ViewManagerService> request) override;

  // mojo::InterfaceFactory<mojo::WindowManagerInternalClient>:
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::WindowManagerInternalClient> request)
      override;

  mojo::ApplicationImpl* app_impl_;
  ViewManagerRootConnectionObserver* observer_;
  scoped_ptr<mojo::Binding<mojo::WindowManagerInternalClient>>
      wm_internal_client_binding_;
  mojo::InterfaceRequest<mojo::ViewManagerClient> wm_internal_client_request_;
  mojo::WindowManagerInternalPtr wm_internal_;
  scoped_ptr<ConnectionManager> connection_manager_;

  DISALLOW_COPY_AND_ASSIGN(ViewManagerRootConnection);
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_VIEW_MANAGER_ROOT_CONNECTION_H_
