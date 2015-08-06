// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/keyboard_native/keyboard_service_impl.h"

namespace keyboard {

KeyboardServiceImpl::KeyboardServiceImpl(
    mojo::InterfaceRequest<KeyboardService> request)
    : strong_binding_(this, request.Pass()), client_() {
}

KeyboardServiceImpl::~KeyboardServiceImpl() {
}

// KeyboardService implementation.
void KeyboardServiceImpl::Show(KeyboardClientPtr client, KeyboardType type) {
  client_ = client.Pass();
}

void KeyboardServiceImpl::ShowByRequest() {
}

void KeyboardServiceImpl::Hide() {
}

void KeyboardServiceImpl::OnKey(const char* key) {
  client_->CommitText(key, 1);
}

void KeyboardServiceImpl::OnDelete() {
  client_->DeleteSurroundingText(1, 0);
}

}  // namespace keyboard
