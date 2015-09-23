// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/kiosk_wm/kiosk_wm_controller.h"

#include "services/kiosk_wm/merged_service_provider.h"
#include "services/window_manager/basic_focus_rules.h"
#include "services/window_manager/window_manager_root.h"

namespace kiosk_wm {

KioskWMController::KioskWMController(window_manager::WindowManagerRoot* wm_root,
                                     std::string default_url)
    : window_manager_root_(wm_root),
      root_(nullptr),
      content_(nullptr),
      default_url_(default_url),
      navigator_host_(this),
      weak_factory_(this) {
  exposed_services_impl_.AddService(this);
}

KioskWMController::~KioskWMController() {}

base::WeakPtr<KioskWMController> KioskWMController::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void KioskWMController::OnEmbed(
    mojo::View* root,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  // KioskWMController does not support being embedded more than once.
  CHECK(!root_);

  root_ = root;
  root_->AddObserver(this);

  // Resize to match the Nexus 5 aspect ratio:
  window_manager_root_->SetViewportSize(gfx::Size(320, 640));

  content_ = root->view_manager()->CreateView();
  content_->SetBounds(root_->bounds());
  root_->AddChild(content_);
  content_->SetVisible(true);

  window_manager_root_->InitFocus(
      make_scoped_ptr(new window_manager::BasicFocusRules(root_)));
  window_manager_root_->accelerator_manager()->Register(
      ui::Accelerator(ui::VKEY_BROWSER_BACK, 0),
      ui::AcceleratorManager::kNormalPriority, this);

  // Now that we're ready, either load a pending url or the default url.
  if (!pending_url_.empty())
    Embed(pending_url_, services.Pass(), exposed_services.Pass());
  else if (!default_url_.empty())
    Embed(default_url_, services.Pass(), exposed_services.Pass());
}

void KioskWMController::Embed(
    const mojo::String& url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  // We can get Embed calls before we've actually been
  // embedded into the root view and content_ is created.
  // Just save the last url, we'll embed it when we're ready.
  if (!content_) {
    pending_url_ = url;
    return;
  }

  merged_service_provider_.reset(
      new MergedServiceProvider(exposed_services.Pass(), this));
  content_->Embed(url, services.Pass(),
                  merged_service_provider_->GetServiceProviderPtr().Pass());

  navigator_host_.RecordNavigation(url);
}

void KioskWMController::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::NavigatorHost> request) {
  navigator_host_.Bind(request.Pass());
}

void KioskWMController::OnViewManagerDisconnected(
    mojo::ViewManager* view_manager) {
  root_ = nullptr;
  delete this;
}

void KioskWMController::OnViewDestroyed(mojo::View* view) {
  view->RemoveObserver(this);
}

void KioskWMController::OnViewBoundsChanged(mojo::View* view,
                                            const mojo::Rect& old_bounds,
                                            const mojo::Rect& new_bounds) {
  content_->SetBounds(new_bounds);
}

// Convenience method:
void KioskWMController::ReplaceContentWithURL(const mojo::String& url) {
  Embed(url, nullptr, nullptr);
}

bool KioskWMController::AcceleratorPressed(const ui::Accelerator& accelerator,
                                           mojo::View* target) {
  if (accelerator.key_code() != ui::VKEY_BROWSER_BACK)
    return false;
  navigator_host_.RequestNavigateHistory(-1);
  return true;
}

bool KioskWMController::CanHandleAccelerators() const {
  return true;
}

}  // namespace kiosk_wm
