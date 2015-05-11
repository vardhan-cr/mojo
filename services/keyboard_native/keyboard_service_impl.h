// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"

namespace keyboard {

class KeyboardServiceImpl : public KeyboardService {
 public:
  explicit KeyboardServiceImpl(mojo::InterfaceRequest<KeyboardService> request);
  ~KeyboardServiceImpl() override;

  // KeyboardService implementation.
  void Show(KeyboardClientPtr client) override;
  void ShowByRequest() override;
  void Hide() override;

 private:
  mojo::StrongBinding<KeyboardService> strong_binding_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardServiceImpl);
};

}  // namespace keyboard
