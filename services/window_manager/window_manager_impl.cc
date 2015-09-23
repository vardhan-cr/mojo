// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/window_manager/window_manager_impl.h"

#include "base/bind.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "services/window_manager/capture_controller.h"
#include "services/window_manager/focus_controller.h"
#include "services/window_manager/window_manager_root.h"

using mojo::Callback;
using mojo::Id;

namespace window_manager {

WindowManagerImpl::WindowManagerImpl(
    WindowManagerRoot* window_manager,
    mojo::ScopedMessagePipeHandle window_manager_pipe,
    bool from_vm)
    : window_manager_(window_manager),
      from_vm_(from_vm),
      binding_(this, window_manager_pipe.Pass()) {
  binding_.set_connection_error_handler(
      // WindowManagerRoot::RemoveConnectedService will destroy this object.
      base::Bind(&WindowManagerRoot::RemoveConnectedService,
                 base::Unretained(window_manager_), base::Unretained(this)));
}

WindowManagerImpl::~WindowManagerImpl(){};

void WindowManagerImpl::NotifyViewFocused(Id focused_id) {
  if (from_vm_ && observer_)
    observer_->OnFocusChanged(focused_id);
}

void WindowManagerImpl::NotifyWindowActivated(Id active_id) {
  if (from_vm_ && observer_)
    observer_->OnActiveWindowChanged(active_id);
}

void WindowManagerImpl::NotifyCaptureChanged(Id capture_id) {
  if (from_vm_ && observer_)
    observer_->OnCaptureChanged(capture_id);
}

void WindowManagerImpl::Embed(
    const mojo::String& url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  window_manager_->Embed(url, services.Pass(), exposed_services.Pass());
}

void WindowManagerImpl::SetCapture(Id view,
                                   const Callback<void(bool)>& callback) {
  callback.Run(from_vm_ && window_manager_->IsReady() &&
               window_manager_->SetCapture(view));
}

void WindowManagerImpl::FocusWindow(Id view,
                                    const Callback<void(bool)>& callback) {
  callback.Run(from_vm_ && window_manager_->IsReady() &&
               window_manager_->FocusWindow(view));
}

void WindowManagerImpl::ActivateWindow(Id view,
                                       const Callback<void(bool)>& callback) {
  callback.Run(from_vm_ && window_manager_->IsReady() &&
               window_manager_->ActivateWindow(view));
}

void WindowManagerImpl::GetFocusedAndActiveViews(
    mojo::WindowManagerObserverPtr observer,
    const mojo::WindowManager::GetFocusedAndActiveViewsCallback& callback) {
  observer_ = observer.Pass();
  if (!window_manager_->focus_controller()) {
    // TODO(sky): add typedef for 0.
    callback.Run(0, 0, 0);
    return;
  }
  mojo::View* capture_view =
      window_manager_->capture_controller()->GetCapture();
  mojo::View* active_view =
      window_manager_->focus_controller()->GetActiveView();
  mojo::View* focused_view =
      window_manager_->focus_controller()->GetFocusedView();
  // TODO(sky): sanitize ids for client.
  callback.Run(capture_view ? capture_view->id() : 0,
               focused_view ? focused_view->id() : 0,
               active_view ? active_view->id() : 0);
}

}  // namespace window_manager
