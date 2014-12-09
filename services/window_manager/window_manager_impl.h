// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_IMPL_H_
#define SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_IMPL_H_

#include "base/basictypes.h"
#include "base/logging.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/services/public/cpp/view_manager/types.h"
#include "mojo/services/window_manager/public/interfaces/window_manager.mojom.h"

namespace window_manager {

class WindowManagerApp;

class WindowManagerImpl : public mojo::WindowManager,
                          public mojo::ErrorHandler {
 public:
  // See description above |from_vm_| for details on |from_vm|.
  // WindowManagerImpl deletes itself on connection errors.  WindowManagerApp
  // also deletes WindowManagerImpl in its destructor.
  WindowManagerImpl(WindowManagerApp* window_manager, bool from_vm);
  ~WindowManagerImpl() override;

  void Bind(mojo::ScopedMessagePipeHandle window_manager_pipe);

  void NotifyViewFocused(mojo::Id new_focused_id, mojo::Id old_focused_id);
  void NotifyWindowActivated(mojo::Id new_active_id, mojo::Id old_active_id);

 private:
  mojo::WindowManagerClient* client() {
    DCHECK(from_vm_);
    return binding_.client();
  }

  // mojo::WindowManager:
  void Embed(
      const mojo::String& url,
      mojo::InterfaceRequest<mojo::ServiceProvider> service_provider) override;
  void SetCapture(uint32_t view_id,
                  const mojo::Callback<void(bool)>& callback) override;
  void FocusWindow(uint32_t view_id,
                   const mojo::Callback<void(bool)>& callback) override;
  void ActivateWindow(uint32_t view_id,
                      const mojo::Callback<void(bool)>& callback) override;

  // mojo::ErrorHandler:
  void OnConnectionError() override;

  WindowManagerApp* window_manager_;

  // Whether this connection originated from the ViewManager. Connections that
  // originate from the view manager are expected to have clients. Connections
  // that don't originate from the view manager do not have clients.
  const bool from_vm_;

  mojo::Binding<mojo::WindowManager> binding_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerImpl);
};

}  // namespace window_manager

#endif  // SERVICES_WINDOW_MANAGER_WINDOW_MANAGER_IMPL_H_
