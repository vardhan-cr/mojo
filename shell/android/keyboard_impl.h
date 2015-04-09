// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_KEYBOARD_IMPL_H_
#define SHELL_ANDROID_KEYBOARD_IMPL_H_

#include <jni.h>

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"

namespace shell {

class KeyboardImpl : public mojo::Keyboard {
 public:
  explicit KeyboardImpl(mojo::InterfaceRequest<mojo::Keyboard> request);
  ~KeyboardImpl();

  // mojo::Keyboard implementation
  void Show() override;
  void Hide() override;

 private:
  mojo::StrongBinding<mojo::Keyboard> binding_;
};

bool RegisterKeyboardJni(JNIEnv* env);

}  // namespace shell

#endif  // SHELL_ANDROID_KEYBOARD_IMPL_H_
