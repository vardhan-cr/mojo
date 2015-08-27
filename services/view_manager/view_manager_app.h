// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_VIEW_MANAGER_APP_H_
#define SERVICES_VIEW_MANAGER_VIEW_MANAGER_APP_H_

#include "base/memory/scoped_ptr.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "services/view_manager/view_manager_root_connection.h"

namespace view_manager {

class ViewManagerApp
    : public mojo::ApplicationDelegate,
      public ViewManagerRootConnection::ViewManagerRootConnectionObserver {
 public:
  ViewManagerApp();
  ~ViewManagerApp() override;

 private:
  // ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // ViewManagerRootConnectionObserver:
  void OnCloseViewManagerRootConnection(
      ViewManagerRootConnection* view_manager_root_connection) override;

  std::set<ViewManagerRootConnection*> active_root_connections_;
  mojo::ApplicationImpl* app_impl_;
  mojo::TracingImpl tracing_;

  DISALLOW_COPY_AND_ASSIGN(ViewManagerApp);
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_VIEW_MANAGER_APP_H_
