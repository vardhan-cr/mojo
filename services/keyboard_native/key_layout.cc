// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/macros.h"
#include "mojo/tools/embed/data.h"
#include "services/keyboard_native/kActionIcon.h"
#include "services/keyboard_native/kDeleteIcon.h"
#include "services/keyboard_native/kLowerCaseIcon.h"
#include "services/keyboard_native/kUpperCaseIcon.h"
#include "services/keyboard_native/key_layout.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImageDecoder.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/rect_f.h"

namespace keyboard {

// An implementation of Key that draws itself as ASCII text.
class KeyLayout::TextKey : public Key {
 public:
  TextKey(const char* text,
          base::Callback<void(const TextKey&)> touch_up_callback)
      : text_(text), touch_up_callback_(touch_up_callback) {}

  ~TextKey() override {}

  void Draw(SkCanvas* canvas,
            const SkPaint& paint,
            const gfx::RectF& rect) override {
    float text_baseline_offset = rect.height() / 5.0f;
    canvas->drawText(text_, strlen(text_), rect.x() + (rect.width() / 2.0f),
                     rect.y() + rect.height() - text_baseline_offset, paint);
  }

  const char* ToText() const override { return text_; }

  void OnTouchUp() override { touch_up_callback_.Run(*this); }

 private:
  const char* text_;
  base::Callback<void(const TextKey&)> touch_up_callback_;

  DISALLOW_COPY_AND_ASSIGN(TextKey);
};

// An implementation of Key that draws itself as an image.
class ImageKey : public KeyLayout::Key {
 public:
  ImageKey(const char* text,
           base::Callback<void(const KeyLayout::TextKey&)> touch_up_callback,
           const mojo::embed::Data& data)
      : text_key_(text, touch_up_callback), bitmap_valid_(false), bitmap_() {
    bool result = gfx::PNGCodec::Decode(
        reinterpret_cast<const unsigned char*>(data.data), data.size, &bitmap_);
    bitmap_valid_ = result && bitmap_.width() > 0 && bitmap_.height() > 0;
    DCHECK(bitmap_valid_);
  }
  ~ImageKey() override {}

  // Key implementation.
  void Draw(SkCanvas* canvas,
            const SkPaint& paint,
            const gfx::RectF& rect) override {
    // If our bitmap is somehow invalid, default to drawing the text of the key.
    if (!bitmap_valid_) {
      text_key_.Draw(canvas, paint, rect);
      return;
    }

    float width_scale = rect.width() / bitmap_.width();
    float height_scale = rect.height() / bitmap_.height();
    float scale = width_scale > height_scale ? height_scale : width_scale;
    float target_width = bitmap_.width() * scale;
    float target_height = bitmap_.height() * scale;
    float delta_width = rect.width() - target_width;
    float target_x = rect.x() + (delta_width / 2.0f);
    float delta_height = rect.height() - target_height;
    float target_y = rect.y() + (delta_height / 2.0f);
    canvas->drawBitmapRect(
        bitmap_,
        SkRect::MakeXYWH(target_x, target_y, target_width, target_height),
        &paint);
  }
  const char* ToText() const override { return text_key_.ToText(); }
  void OnTouchUp() override { text_key_.OnTouchUp(); }

 private:
  KeyLayout::TextKey text_key_;
  bool bitmap_valid_;
  SkBitmap bitmap_;

  DISALLOW_COPY_AND_ASSIGN(ImageKey);
};

KeyLayout::KeyLayout()
    : on_text_callback_(),
      layout_(&letters_layout_),
      key_map_(&lower_case_key_map_),
      weak_factory_(this) {
  InitLayouts();
  InitKeyMaps();
}

KeyLayout::~KeyLayout() {
  for (auto& row : lower_case_key_map_) {
    for (auto& key : row) {
      delete key;
    }
  }
  for (auto& row : upper_case_key_map_) {
    for (auto& key : row) {
      delete key;
    }
  }
  for (auto& row : symbols_key_map_) {
    for (auto& key : row) {
      delete key;
    }
  }
}

void KeyLayout::SetTextCallback(
    base::Callback<void(const std::string&)> on_text_callback) {
  on_text_callback_ = on_text_callback;
}

void KeyLayout::SetDeleteCallback(base::Callback<void()> on_delete_callback) {
  on_delete_callback_ = on_delete_callback;
}

void KeyLayout::SetSize(const mojo::Size& size) {
  size_ = size;
}

void KeyLayout::Draw(SkCanvas* canvas) {
  float row_height =
      static_cast<float>(size_.height) / static_cast<float>(layout_->size());

  skia::RefPtr<SkTypeface> typeface =
      skia::AdoptRef(SkTypeface::CreateFromName("Arial", SkTypeface::kNormal));
  SkPaint text_paint;
  text_paint.setTypeface(typeface.get());
  text_paint.setColor(SK_ColorBLACK);
  text_paint.setTextSize(row_height / 2);
  text_paint.setAntiAlias(true);
  text_paint.setTextAlign(SkPaint::kCenter_Align);

  canvas->clear(SK_ColorLTGRAY);

  SkPaint paint;
  for (size_t row_index = 0; row_index < layout_->size(); row_index++) {
    float current_top = row_index * row_height;
    float current_left = 0;
    for (size_t key_index = 0; key_index < (*layout_)[row_index].size();
         key_index++) {
      float key_width =
          static_cast<float>(size_.width) * (*layout_)[row_index][key_index];

      (*key_map_)[row_index][key_index]->Draw(
          canvas, text_paint,
          gfx::RectF(current_left, current_top, key_width, row_height));
      current_left += key_width;
    }
  }
}

KeyLayout::Key* KeyLayout::GetKeyAtPoint(const gfx::Point& point) {
  if (point.x() < 0 || point.y() < 0 || point.x() >= size_.width ||
      point.y() >= size_.height) {
    return nullptr;
  }

  int row_index = point.y() / (size_.height / layout_->size());
  float width_percent =
      static_cast<float>(point.x()) / static_cast<float>(size_.width);

  int key_index = 0;
  while (width_percent >= (*layout_)[row_index][key_index]) {
    width_percent -= (*layout_)[row_index][key_index];
    key_index++;
  }
  return (*key_map_)[row_index][key_index];
}

void KeyLayout::OnTouchUp(const gfx::Point& touch_up) {
  Key* key = GetKeyAtPoint(touch_up);
  if (key != nullptr) {
    key->OnTouchUp();
  }
}

void KeyLayout::InitLayouts() {
  // Row layouts are specified by a vector of floats which indicate the percent
  // width a given key takes up in that row.  The floats of a given row *MUST*
  // add up to 1.
  std::vector<float> ten_key_row_layout = {
      0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f};
  std::vector<float> nine_key_row_layout = {
      0.15f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.15f};
  std::vector<float> seven_key_row_layout = {
      0.15f, 0.1f, 0.1f, 0.3f, 0.1f, 0.1f, 0.15f};
  std::vector<float> five_key_row_layout = {0.15f, 0.1f, 0.5f, 0.1f, 0.15f};

  letters_layout_.push_back(ten_key_row_layout);
  letters_layout_.push_back(nine_key_row_layout);
  letters_layout_.push_back(nine_key_row_layout);
  letters_layout_.push_back(five_key_row_layout);

  symbols_layout_.push_back(ten_key_row_layout);
  symbols_layout_.push_back(nine_key_row_layout);
  symbols_layout_.push_back(nine_key_row_layout);
  symbols_layout_.push_back(seven_key_row_layout);
}

void KeyLayout::OnKeyDoNothing(const TextKey& key) {
  // do nothing
}

void KeyLayout::OnKeyEmitText(const TextKey& key) {
  on_text_callback_.Run(std::string(key.ToText()));
}

void KeyLayout::OnKeyDelete(const TextKey& key) {
  on_delete_callback_.Run();
}

void KeyLayout::OnKeySwitchToUpperCase(const TextKey& key) {
  layout_ = &letters_layout_;
  key_map_ = &upper_case_key_map_;
}

void KeyLayout::OnKeySwitchToLowerCase(const TextKey& key) {
  layout_ = &letters_layout_;
  key_map_ = &lower_case_key_map_;
}

void KeyLayout::OnKeySwitchToSymbols(const TextKey& key) {
  layout_ = &symbols_layout_;
  key_map_ = &symbols_key_map_;
}

void KeyLayout::InitKeyMaps() {
  base::Callback<void(const TextKey&)> do_nothing_callback =
      base::Bind(&KeyLayout::OnKeyDoNothing, weak_factory_.GetWeakPtr());
  base::Callback<void(const TextKey&)> emit_text_callback =
      base::Bind(&KeyLayout::OnKeyEmitText, weak_factory_.GetWeakPtr());
  base::Callback<void(const TextKey&)> delete_callback =
      base::Bind(&KeyLayout::OnKeyDelete, weak_factory_.GetWeakPtr());
  base::Callback<void(const TextKey&)> switch_to_upper_case_callback =
      base::Bind(&KeyLayout::OnKeySwitchToUpperCase,
                 weak_factory_.GetWeakPtr());
  base::Callback<void(const TextKey&)> switch_to_lower_case_callback =
      base::Bind(&KeyLayout::OnKeySwitchToLowerCase,
                 weak_factory_.GetWeakPtr());
  base::Callback<void(const TextKey&)> switch_to_symbols_callback =
      base::Bind(&KeyLayout::OnKeySwitchToSymbols, weak_factory_.GetWeakPtr());

  ImageKey* switch_to_upper_case_image_key = new ImageKey(
      "/\\", switch_to_upper_case_callback, keyboard_native::kUpperCaseIcon);
  ImageKey* switch_to_lower_case_image_key = new ImageKey(
      "\\/", switch_to_lower_case_callback, keyboard_native::kLowerCaseIcon);
  ImageKey* delete_image_key =
      new ImageKey("<-", delete_callback, keyboard_native::kDeleteIcon);
  ImageKey* action_image_key =
      new ImageKey(":)", do_nothing_callback, keyboard_native::kActionIcon);

  std::vector<Key*> lower_case_key_map_row_one = {
      new TextKey("q", emit_text_callback),
      new TextKey("w", emit_text_callback),
      new TextKey("e", emit_text_callback),
      new TextKey("r", emit_text_callback),
      new TextKey("t", emit_text_callback),
      new TextKey("y", emit_text_callback),
      new TextKey("u", emit_text_callback),
      new TextKey("o", emit_text_callback),
      new TextKey("i", emit_text_callback),
      new TextKey("p", emit_text_callback)};

  std::vector<Key*> lower_case_key_map_row_two = {
      new TextKey("a", emit_text_callback),
      new TextKey("s", emit_text_callback),
      new TextKey("d", emit_text_callback),
      new TextKey("f", emit_text_callback),
      new TextKey("g", emit_text_callback),
      new TextKey("h", emit_text_callback),
      new TextKey("j", emit_text_callback),
      new TextKey("k", emit_text_callback),
      new TextKey("l", emit_text_callback)};

  std::vector<Key*> lower_case_key_map_row_three = {
      switch_to_upper_case_image_key,
      new TextKey("z", emit_text_callback),
      new TextKey("x", emit_text_callback),
      new TextKey("c", emit_text_callback),
      new TextKey("v", emit_text_callback),
      new TextKey("b", emit_text_callback),
      new TextKey("n", emit_text_callback),
      new TextKey("m", emit_text_callback),
      delete_image_key};

  std::vector<Key*> lower_case_key_map_row_four = {
      new TextKey("sym", switch_to_symbols_callback),
      new TextKey(",", emit_text_callback),
      new TextKey(" ", emit_text_callback),
      new TextKey(".", emit_text_callback),
      action_image_key};

  lower_case_key_map_ = {lower_case_key_map_row_one,
                         lower_case_key_map_row_two,
                         lower_case_key_map_row_three,
                         lower_case_key_map_row_four};

  std::vector<Key*> upper_case_key_map_row_one = {
      new TextKey("Q", emit_text_callback),
      new TextKey("W", emit_text_callback),
      new TextKey("E", emit_text_callback),
      new TextKey("R", emit_text_callback),
      new TextKey("T", emit_text_callback),
      new TextKey("Y", emit_text_callback),
      new TextKey("U", emit_text_callback),
      new TextKey("O", emit_text_callback),
      new TextKey("I", emit_text_callback),
      new TextKey("P", emit_text_callback)};

  std::vector<Key*> upper_case_key_map_row_two = {
      new TextKey("A", emit_text_callback),
      new TextKey("S", emit_text_callback),
      new TextKey("D", emit_text_callback),
      new TextKey("F", emit_text_callback),
      new TextKey("G", emit_text_callback),
      new TextKey("H", emit_text_callback),
      new TextKey("J", emit_text_callback),
      new TextKey("K", emit_text_callback),
      new TextKey("L", emit_text_callback)};

  std::vector<Key*> upper_case_key_map_row_three = {
      switch_to_lower_case_image_key,
      new TextKey("Z", emit_text_callback),
      new TextKey("X", emit_text_callback),
      new TextKey("C", emit_text_callback),
      new TextKey("V", emit_text_callback),
      new TextKey("B", emit_text_callback),
      new TextKey("N", emit_text_callback),
      new TextKey("M", emit_text_callback),
      delete_image_key};

  std::vector<Key*> upper_case_key_map_row_four = {
      new TextKey("SYM", switch_to_symbols_callback),
      new TextKey(",", emit_text_callback),
      new TextKey(" ", emit_text_callback),
      new TextKey(".", emit_text_callback),
      action_image_key};

  upper_case_key_map_ = {upper_case_key_map_row_one,
                         upper_case_key_map_row_two,
                         upper_case_key_map_row_three,
                         upper_case_key_map_row_four};

  std::vector<Key*> symbols_key_map_row_one = {
      new TextKey("1", emit_text_callback),
      new TextKey("2", emit_text_callback),
      new TextKey("3", emit_text_callback),
      new TextKey("4", emit_text_callback),
      new TextKey("5", emit_text_callback),
      new TextKey("6", emit_text_callback),
      new TextKey("7", emit_text_callback),
      new TextKey("8", emit_text_callback),
      new TextKey("9", emit_text_callback),
      new TextKey("0", emit_text_callback)};

  std::vector<Key*> symbols_key_map_row_two = {
      new TextKey("@", emit_text_callback),
      new TextKey("#", emit_text_callback),
      new TextKey("$", emit_text_callback),
      new TextKey("%", emit_text_callback),
      new TextKey("&", emit_text_callback),
      new TextKey("-", emit_text_callback),
      new TextKey("+", emit_text_callback),
      new TextKey("(", emit_text_callback),
      new TextKey(")", emit_text_callback)};

  std::vector<Key*> symbols_key_map_row_three = {
      new TextKey("=\\<", switch_to_symbols_callback),
      new TextKey("*", emit_text_callback),
      new TextKey("\"", emit_text_callback),
      new TextKey("'", emit_text_callback),
      new TextKey(":", emit_text_callback),
      new TextKey(";", emit_text_callback),
      new TextKey("!", emit_text_callback),
      new TextKey("?", emit_text_callback),
      delete_image_key};

  std::vector<Key*> symbols_key_map_row_four = {
      new TextKey("ABC", switch_to_lower_case_callback),
      new TextKey(",", emit_text_callback),
      new TextKey("_", emit_text_callback),
      new TextKey(" ", emit_text_callback),
      new TextKey("/", emit_text_callback),
      new TextKey(".", emit_text_callback),
      action_image_key};

  symbols_key_map_ = {symbols_key_map_row_one,
                      symbols_key_map_row_two,
                      symbols_key_map_row_three,
                      symbols_key_map_row_four};
}
}
// namespace keyboard
