// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/keyboard_native/keyboard_service_impl.h"

namespace keyboard {

KeyboardServiceImpl::KeyboardServiceImpl(
    mojo::InterfaceRequest<KeyboardService> request)
    : strong_binding_(this, request.Pass()) {
}

KeyboardServiceImpl::~KeyboardServiceImpl() {
}

// KeyboardService implementation.
void KeyboardServiceImpl::Show(KeyboardClientPtr client) {
  keyboard::CompletionData completion_data;
  completion_data.text = "blah";
  completion_data.label = "blahblah";
  client->CommitCompletion(completion_data.Clone());

  keyboard::CorrectionData correction_data;
  correction_data.old_text = "old text";
  correction_data.new_text = "new text";
  client->CommitCorrection(correction_data.Clone());

  client->CommitText("", 0);
  client->DeleteSurroundingText(0, 0);
  client->SetComposingRegion(0, 0);
  client->SetComposingText("", 0);
  client->SetSelection(0, 1);
}

void KeyboardServiceImpl::ShowByRequest() {
}

void KeyboardServiceImpl::Hide() {
}

// mojo::ViewObserver implementation.
void KeyboardServiceImpl::OnViewInputEvent(mojo::View* view,
                                           const mojo::EventPtr& event) {
}

}  // namespace keyboard
