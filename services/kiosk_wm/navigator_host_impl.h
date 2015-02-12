// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_
#define SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/navigation/public/interfaces/navigation.mojom.h"

namespace kiosk_wm {
class KioskWM;

class NavigatorHostImpl : public mojo::NavigatorHost {
 public:
  NavigatorHostImpl(KioskWM* kiosk_wm,
                    mojo::InterfaceRequest<mojo::NavigatorHost> request);
  ~NavigatorHostImpl();

 private:
  void DidNavigateLocally(const mojo::String& url) override;
  void RequestNavigate(mojo::Target target,
                       mojo::URLRequestPtr request) override;

  base::WeakPtr<KioskWM> kiosk_wm_;
  mojo::StrongBinding<NavigatorHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(NavigatorHostImpl);
};

}  // namespace kiosk_wm

#endif  // SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_
