// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/window_manager/window_manager_impl.h"

#include "mojo/services/view_manager/public/cpp/view.h"
#include "services/window_manager/focus_controller.h"
#include "services/window_manager/window_manager_app.h"

using mojo::Callback;
using mojo::Id;

namespace window_manager {

WindowManagerImpl::WindowManagerImpl(WindowManagerApp* window_manager,
                                     bool from_vm)
    : window_manager_(window_manager), from_vm_(from_vm), binding_(this) {
  window_manager_->AddConnection(this);
  binding_.set_error_handler(this);
}

WindowManagerImpl::~WindowManagerImpl() {
  window_manager_->RemoveConnection(this);
}

void WindowManagerImpl::Bind(
    mojo::ScopedMessagePipeHandle window_manager_pipe) {
  binding_.Bind(window_manager_pipe.Pass());
}

void WindowManagerImpl::NotifyViewFocused(Id new_focused_id,
                                          Id old_focused_id) {
  if (from_vm_)
    client()->OnFocusChanged(old_focused_id, new_focused_id);
}

void WindowManagerImpl::NotifyWindowActivated(Id new_active_id,
                                              Id old_active_id) {
  if (from_vm_)
    client()->OnActiveWindowChanged(old_active_id, new_active_id);
}

void WindowManagerImpl::NotifyCaptureChanged(Id new_capture_id,
                                             Id old_capture_id) {
  if (from_vm_)
    client()->OnCaptureChanged(old_capture_id, new_capture_id);
}

void WindowManagerImpl::Embed(
    const mojo::String& url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  window_manager_->Embed(url, services.Pass(), exposed_services.Pass());
}

void WindowManagerImpl::SetCapture(Id view,
                                   const Callback<void(bool)>& callback) {
  if (!from_vm_)
    return;  // See comments for |from_vm_| on this.

  bool success = window_manager_->IsReady();
  if (success)
    window_manager_->SetCapture(view);
  callback.Run(success);
}

void WindowManagerImpl::FocusWindow(Id view,
                                    const Callback<void(bool)>& callback) {
  if (!from_vm_)
    return;  // See comments for |from_vm_| on this.

  bool success = window_manager_->IsReady();
  if (success)
    window_manager_->FocusWindow(view);
  callback.Run(success);
}

void WindowManagerImpl::ActivateWindow(Id view,
                                       const Callback<void(bool)>& callback) {
  if (!from_vm_)
    return;  // See comments for |from_vm_| on this.

  bool success = window_manager_->IsReady();
  if (success)
    window_manager_->ActivateWindow(view);
  callback.Run(success);
}

void WindowManagerImpl::GetFocusedAndActiveViews(
    const mojo::Callback<void(uint32_t, uint32_t)>& callback) {
  if (!window_manager_->focus_controller()) {
    // TODO(sky): add typedef for 0.
    callback.Run(0, 0);
    return;
  }
  mojo::View* active_view =
      window_manager_->focus_controller()->GetActiveView();
  mojo::View* focused_view =
      window_manager_->focus_controller()->GetFocusedView();
  // TODO(sky): sanitize ids for client.
  callback.Run(focused_view ? focused_view->id() : 0,
               active_view ? active_view->id() : 0);
}

void WindowManagerImpl::OnConnectionError() {
  delete this;
}

}  // namespace window_manager
