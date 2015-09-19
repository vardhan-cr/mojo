// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_TEXT_UPDATE_KEY_H_
#define SERVICES_KEYBOARD_NATIVE_TEXT_UPDATE_KEY_H_

#include <string>

#include "base/callback.h"
#include "services/keyboard_native/key_layout.h"

class SkCanvas;
class SkPaint;

namespace gfx {
class RectF;
}

namespace keyboard {
class TextUpdateKey : public KeyLayout::Key {
 public:
  TextUpdateKey(std::string text,
                base::Callback<void(const TextUpdateKey&)> touch_up_callback);

  ~TextUpdateKey() override;

  // Key implementation.
  void Draw(SkCanvas* canvas,
            const SkPaint& paint,
            const gfx::RectF& rect) override;

  const char* ToText() const override;

  void OnTouchUp() override;

  void ChangeText(std::string new_text);

 private:
  std::string text_;
  base::Callback<void(const TextUpdateKey&)> touch_up_callback_;

  DISALLOW_COPY_AND_ASSIGN(TextUpdateKey);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_TEXT_UPDATE_KEY_H_
