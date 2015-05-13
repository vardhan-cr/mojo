// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_
#define SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "services/keyboard_native/view_observer_delegate.h"

namespace keyboard {

class KeyboardServiceImpl : public KeyboardService, public mojo::ViewObserver {
 public:
  explicit KeyboardServiceImpl(mojo::InterfaceRequest<KeyboardService> request);
  ~KeyboardServiceImpl() override;

  // KeyboardService implementation.
  void Show(KeyboardClientPtr client) override;
  void ShowByRequest() override;
  void Hide() override;

  // mojo::ViewObserver implementation.
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

 private:
  mojo::StrongBinding<KeyboardService> strong_binding_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceImpl);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_
