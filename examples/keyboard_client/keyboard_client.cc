// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"

namespace examples {

class KeyboardDelegate : public mojo::ApplicationDelegate,
                         public keyboard::KeyboardClient {
 public:
  KeyboardDelegate() : binding_(this) {}

  ~KeyboardDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    app->ConnectToService("mojo:keyboard_native", &keyboard_);
    keyboard_->ShowByRequest();
    keyboard_->Hide();

    keyboard::KeyboardClientPtr keyboard_client;
    auto request = mojo::GetProxy(&keyboard_client);
    binding_.Bind(request.Pass());
    keyboard_->Show(keyboard_client.Pass());
  }

  // keyboard::KeyboardClient implementation.
  void CommitCompletion(keyboard::CompletionDataPtr completion) override {}

  void CommitCorrection(keyboard::CorrectionDataPtr correction) override {}

  void CommitText(const mojo::String& text,
                  int32_t new_cursor_position) override {}

  void DeleteSurroundingText(int32_t before_length,
                             int32_t after_length) override {}

  void SetComposingRegion(int32_t start, int32_t end) override {}

  void SetComposingText(const mojo::String& text,
                        int32_t new_cursor_position) override {}

  void SetSelection(int32_t start, int32_t end) override {}

 private:
  mojo::Binding<keyboard::KeyboardClient> binding_;
  keyboard::KeyboardServicePtr keyboard_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new examples::KeyboardDelegate);
  return runner.Run(application_request);
}
