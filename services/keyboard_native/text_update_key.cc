// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "services/keyboard_native/text_update_key.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkPaint.h"
#include "third_party/skia/include/core/SkScalar.h"
#include "ui/gfx/geometry/rect_f.h"

namespace keyboard {

TextUpdateKey::TextUpdateKey(
    std::string text,
    base::Callback<void(const TextUpdateKey&)> touch_up_callback)
    : text_(text), touch_up_callback_(touch_up_callback) {
}

TextUpdateKey::~TextUpdateKey() {
}

// Key implementation.
void TextUpdateKey::Draw(SkCanvas* canvas,
                         const SkPaint& paint,
                         const gfx::RectF& rect) {
  std::string text_to_fit = text_;
  SkRect bounds;
  SkPaint paint_copy = paint;
  paint_copy.measureText((const void*)(text_.c_str()), strlen(text_.c_str()),
                         &bounds);
  bool text_need_scale = false;
  if (bounds.width() > rect.width() * 0.8) {
    text_need_scale = true;

    paint_copy.setTextScaleX(SkFloatToScalar(0.6));
    paint_copy.measureText((const void*)(text_.c_str()), strlen(text_.c_str()),
                           &bounds);
    paint_copy.setTextScaleX(SkIntToScalar(1));
    if (bounds.width() > rect.width() * 0.8) {
      int dot_count = SkScalarTruncToInt((SkScalarToFloat(bounds.width()) -
                                          SkScalarToFloat(rect.width()) * 0.8) /
                                         SkScalarToFloat(bounds.width()) *
                                         strlen(text_.c_str())) +
                      1;
      int dot_count_in_text = dot_count < 3 ? dot_count : 3;
      std::string dots(dot_count_in_text, '.');
      text_to_fit =
          dots +
          text_to_fit.substr(dot_count, text_to_fit.length() - dot_count);
    }
  }

  float text_baseline_offset = rect.height() / 5.0f;
  if (text_need_scale) {
    paint_copy.setTextScaleX(SkFloatToScalar(0.6));
  }
  canvas->drawText(text_to_fit.c_str(), strlen(text_to_fit.c_str()),
                   rect.x() + (rect.width() / 2.0f),
                   rect.y() + rect.height() - text_baseline_offset, paint_copy);
  paint_copy.setTextScaleX(SkIntToScalar(1));
}

const char* TextUpdateKey::ToText() const {
  const char* text_char = text_.c_str();
  return text_char;
}

void TextUpdateKey::OnTouchUp() {
  touch_up_callback_.Run(*this);
}

void TextUpdateKey::ChangeText(std::string new_text) {
  text_ = new_text;
}

}  // namespace keyboard
