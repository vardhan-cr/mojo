// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/wm_flow/wm/frame_controller.h"

#include "base/macros.h"
#include "base/strings/utf_string_conversions.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/public/interfaces/application/service_provider.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "services/window_manager/capture_controller.h"
#include "services/window_manager/window_manager_root.h"
#include "url/gurl.h"

////////////////////////////////////////////////////////////////////////////////
// FrameController, public:

FrameController::FrameController(
    const GURL& frame_app_url,
    mojo::View* view,
    mojo::View** app_view,
    window_manager::WindowManagerRoot* window_manager_root)
    : view_(view),
      app_view_(view->view_manager()->CreateView()),
      maximized_(false),
      window_manager_root_(window_manager_root),
      binding_(this) {
  view_->AddObserver(this);
  view_->SetVisible(true);  // FIXME: This should not be our responsibility?
  *app_view = app_view_;

  viewer_services_impl_.AddService(this);
  mojo::ServiceProviderPtr viewer_services;
  viewer_services_impl_.Bind(GetProxy(&viewer_services));

  view_->Embed(frame_app_url.spec(), nullptr, viewer_services.Pass());

  // We weren't observing when our initial bounds was set:
  OnViewBoundsChanged(view, view->bounds(), view->bounds());

  // Add the child view after embedding sky, since embed clears children.
  view_->AddChild(app_view_);
  app_view_->SetVisible(true);
}

FrameController::~FrameController() {}

void FrameController::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<examples::WindowFrameHost> request) {
  binding_.Bind(request.Pass());
}

void FrameController::CloseWindow() {
  // This destroys |app_view_| as it is a child of |view_|.
  view_->Destroy();
}

void FrameController::ToggleMaximize() {
  if (!maximized_)
    restored_bounds_ = view_->bounds().To<gfx::Rect>();
  maximized_ = !maximized_;
  if (maximized_)
    view_->SetBounds(view_->parent()->bounds());
  else
    view_->SetBounds(*mojo::Rect::From(restored_bounds_));
}

void FrameController::ActivateWindow() {
  window_manager_root_->focus_controller()->ActivateView(view_);
}

void FrameController::SetCapture(bool frame_has_capture) {
  if (frame_has_capture)
    window_manager_root_->capture_controller()->SetCapture(view_);
  else
    window_manager_root_->capture_controller()->ReleaseCapture(view_);
}

////////////////////////////////////////////////////////////////////////////////
// FrameController, mojo::ViewObserver implementation:

void FrameController::OnViewDestroyed(mojo::View* view) {
  view_->RemoveObserver(this);
  delete this;
}

void FrameController::OnViewBoundsChanged(mojo::View* view,
                                          const mojo::Rect& old_bounds,
                                          const mojo::Rect& new_bounds) {
  CHECK(view == view_);
  // Statically size the embedded view.  Unclear if we should use a
  // sky iframe to participate in sky layout or not.
  const int kTopControlsAdditionalInset = 15;
  const int kDefaultInset = 25;
  mojo::Rect bounds;
  bounds.x = bounds.y = kDefaultInset;
  bounds.y += kTopControlsAdditionalInset;
  bounds.width = view_->bounds().width - kDefaultInset * 2;
  bounds.height =
      view_->bounds().height - kDefaultInset * 2 - kTopControlsAdditionalInset;
  app_view_->SetBounds(bounds);
}
