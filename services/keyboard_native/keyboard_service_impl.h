// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_
#define SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"
#include "services/keyboard_native/view_observer_delegate.h"

namespace keyboard {

class KeyboardServiceImpl : public KeyboardService {
 public:
  explicit KeyboardServiceImpl(mojo::InterfaceRequest<KeyboardService> request);
  ~KeyboardServiceImpl() override;

  // KeyboardService implementation.
  void Show(KeyboardClientPtr client, KeyboardType type) override;
  void ShowByRequest() override;
  void Hide() override;

  void OnKey(const char* key);
  void OnDelete();

 private:
  mojo::StrongBinding<KeyboardService> strong_binding_;
  KeyboardClientPtr client_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceImpl);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_KEYBOARD_SERVICE_IMPL_H_
