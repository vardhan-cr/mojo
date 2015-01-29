// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_
#define SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_

#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/navigation/public/interfaces/navigation.mojom.h"
#include "sky/tools/debugger/debugger.mojom.h"

namespace mojo {
namespace kiosk_wm {
class KioskWM;

class NavigatorHostImpl : public mojo::InterfaceImpl<mojo::NavigatorHost> {
 public:
  explicit NavigatorHostImpl(KioskWM*);
  ~NavigatorHostImpl();

 private:
  void DidNavigateLocally(const mojo::String& url) override;
  void RequestNavigate(mojo::Target target,
                       mojo::URLRequestPtr request) override;

  base::WeakPtr<KioskWM> kiosk_wm_;

  DISALLOW_COPY_AND_ASSIGN(NavigatorHostImpl);
};

typedef mojo::InterfaceFactoryImplWithContext<NavigatorHostImpl,
                                              KioskWM>
    NavigatorHostFactory;

}  // namespace kiosk_wm
}  // namespace mojo

#endif  // SERVICES_KIOSK_WM_NAVIGATOR_HOST_IMPL_H_
