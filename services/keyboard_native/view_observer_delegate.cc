// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/keyboard_native/view_observer_delegate.h"

#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/skia/ganesh_surface.h"
#include "services/keyboard_native/keyboard_service_impl.h"
#include "third_party/skia/include/core/SkCanvas.h"

namespace keyboard {

mojo::Size ToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

ViewObserverDelegate::ViewObserverDelegate()
    : keyboard_service_impl_(nullptr), view_(nullptr) {
}

ViewObserverDelegate::~ViewObserverDelegate() {
}

void ViewObserverDelegate::SetKeyboardServiceImpl(
    KeyboardServiceImpl* keyboard_service_impl) {
  keyboard_service_impl_ = keyboard_service_impl;
}

void ViewObserverDelegate::OnViewCreated(mojo::View* view, mojo::Shell* shell) {
  if (view_ != nullptr) {
    view_->RemoveObserver(this);
  }
  view_ = view;
  view_->AddObserver(this);
  gl_context_ = mojo::GLContext::Create(shell);
  gr_context_ = std::unique_ptr<mojo::GaneshContext>(
      new mojo::GaneshContext(gl_context_));
  texture_uploader_ = std::unique_ptr<mojo::TextureUploader>(
      new mojo::TextureUploader(this, shell, gl_context_));

  Draw(ToSize(view_->bounds()));
}

void ViewObserverDelegate::Draw(const mojo::Size& size) {
  static const int kKeyColorCount = 2;
  static const int kKeyColors[] = {
      SK_ColorLTGRAY, SK_ColorDKGRAY,
  };

  static const int kRowColors[] = {
      SK_ColorGREEN, SK_ColorYELLOW, SK_ColorMAGENTA, SK_ColorCYAN,
  };
  static const int kRowCount = 4;
  static const int kMaxKeysPerRow = 10;
  static const int kKeyInset = 2;

  float standard_key_width =
      static_cast<float>(size.width) / static_cast<float>(kMaxKeysPerRow);
  float standard_key_height =
      static_cast<float>(size.height) / static_cast<float>(kRowCount);

  mojo::GaneshContext::Scope scope(gr_context_.get());
  mojo::GaneshSurface surface(
      gr_context_.get(),
      make_scoped_ptr(new mojo::GLTexture(gl_context_, size)));

  SkCanvas* canvas = surface.canvas();
  canvas->clear(SK_ColorRED);

  SkPaint paint;

  for (int i = 0; i < kRowCount; i++) {
    paint.setColor(kRowColors[i]);
    SkRect rect = SkRect::MakeLTRB(0, i * standard_key_height, size.width,
                                   (i + 1) * standard_key_height);
    canvas->drawRect(rect, paint);
  }

  // First row
  int current_top = 0;
  int current_left = 0;
  for (int i = 0; i < kMaxKeysPerRow; i++) {
    paint.setColor(kKeyColors[i % kKeyColorCount]);
    SkRect rect = SkRect::MakeLTRB(current_left, current_top,
                                   current_left + standard_key_width,
                                   current_top + standard_key_height);
    rect.inset(kKeyInset, kKeyInset);
    canvas->drawRect(rect, paint);
    current_left += standard_key_width;
  }

  // Second row
  current_top += standard_key_height;
  current_left = 0;
  paint.setColor(kKeyColors[1]);
  SkRect rect = SkRect::MakeLTRB(current_left, current_top,
                                 current_left + (1.5f * standard_key_width),
                                 current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left = 1.5f * standard_key_width;
  for (int i = 0; i < kMaxKeysPerRow - 3; i++) {
    paint.setColor(kKeyColors[i % kKeyColorCount]);
    rect = SkRect::MakeLTRB(current_left, current_top,
                            current_left + standard_key_width,
                            current_top + standard_key_height);
    rect.inset(kKeyInset, kKeyInset);
    canvas->drawRect(rect, paint);
    current_left += standard_key_width;
  }
  paint.setColor(kKeyColors[1]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (1.5f * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);

  // Third row
  current_top += standard_key_height;
  current_left = 0;
  paint.setColor(kKeyColors[0]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (1.5f * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left = 1.5f * standard_key_width;
  for (int i = 0; i < kMaxKeysPerRow - 3; i++) {
    paint.setColor(kKeyColors[(i + 1) % kKeyColorCount]);
    rect = SkRect::MakeLTRB(current_left, current_top,
                            current_left + standard_key_width,
                            current_top + standard_key_height);
    rect.inset(kKeyInset, kKeyInset);
    canvas->drawRect(rect, paint);
    current_left += standard_key_width;
  }
  paint.setColor(kKeyColors[0]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (1.5f * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);

  // Fourth row
  current_top += standard_key_height;
  current_left = 0;
  paint.setColor(kKeyColors[1]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (1.5f * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left += 1.5f * standard_key_width;

  paint.setColor(kKeyColors[0]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + standard_key_width,
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left += standard_key_width;

  paint.setColor(kKeyColors[1]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (5 * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left += (5 * standard_key_width);

  paint.setColor(kKeyColors[0]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + standard_key_width,
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);
  current_left += standard_key_width;

  paint.setColor(kKeyColors[1]);
  rect = SkRect::MakeLTRB(current_left, current_top,
                          current_left + (1.5f * standard_key_width),
                          current_top + standard_key_height);
  rect.inset(kKeyInset, kKeyInset);
  canvas->drawRect(rect, paint);

  canvas->flush();

  texture_uploader_.get()->Upload(surface.TakeTexture());
}

// mojo::TextureUploader::Client implementation.
void ViewObserverDelegate::OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) {
  view_->SetSurfaceId(surface_id.Pass());
}

// mojo::ViewObserver implementation.
void ViewObserverDelegate::OnViewBoundsChanged(mojo::View* view,
                                               const mojo::Rect& old_bounds,
                                               const mojo::Rect& new_bounds) {
  Draw(ToSize(new_bounds));
}

void ViewObserverDelegate::OnViewInputEvent(mojo::View* view,
                                            const mojo::EventPtr& event) {
  if (keyboard_service_impl_) {
    keyboard_service_impl_->OnViewInputEvent(view, event);
  }
  // TODO(anwilson): call KeyboardServiceImpl with key input
}

void ViewObserverDelegate::OnViewDestroyed(mojo::View* view) {
  if (view_ == view) {
    view_->RemoveObserver(this);
    view_ = nullptr;
    gl_context_->Destroy();
  }
}

}  // namespace keyboard
