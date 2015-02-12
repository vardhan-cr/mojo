// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/kiosk_wm/navigator_host_impl.h"

#include "services/kiosk_wm/kiosk_wm.h"

namespace kiosk_wm {

NavigatorHostImpl::NavigatorHostImpl(
    KioskWM* window_manager,
    mojo::InterfaceRequest<mojo::NavigatorHost> request)
    : kiosk_wm_(window_manager->GetWeakPtr()), binding_(this, request.Pass()) {
}

NavigatorHostImpl::~NavigatorHostImpl() {
}

void NavigatorHostImpl::DidNavigateLocally(const mojo::String& url) {
  // TODO(abarth): Do something interesting.
}

void NavigatorHostImpl::RequestNavigate(mojo::Target target,
                                        mojo::URLRequestPtr request) {
  if (!kiosk_wm_)
    return;

  // kiosk_wm sets up default services including navigation.
  kiosk_wm_->ReplaceContentWithURL(request->url);
}

}  // namespace kiosk_wm
