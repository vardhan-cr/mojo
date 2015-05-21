// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "services/keyboard_native/view_observer_delegate.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/sys_info.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/skia/ganesh_surface.h"
#include "services/keyboard_native/keyboard_service_impl.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace keyboard {

static const base::TimeDelta kAnimationDurationMs =
    base::TimeDelta::FromMilliseconds(1000);
static const base::TimeDelta kMinFramePeriodMs =
    base::TimeDelta::FromMillisecondsD(1000.0f / 120.0f);
static const float kMinSegmentLength = 20;

mojo::Size ToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

ViewObserverDelegate::ViewObserverDelegate()
    : keyboard_service_impl_(nullptr),
      view_(nullptr),
      last_action_(-1),
      key_layout_(),
      last_key_(nullptr),
      last_point_(),
      last_point_valid_(false),
      weak_factory_(this) {
  key_layout_.SetTextCallback(
      base::Bind(&ViewObserverDelegate::OnText, weak_factory_.GetWeakPtr()));
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
  gr_context_.reset(new mojo::GaneshContext(gl_context_));
  texture_uploader_.reset(new mojo::TextureUploader(this, shell, gl_context_));

  DrawState();
}

void ViewObserverDelegate::OnText(const std::string& text) {
  keyboard_service_impl_->OnKey(text.c_str());
}

void ViewObserverDelegate::UpdateState(const gfx::Point& touch_point) {
  base::TimeTicks current_ticks = base::TimeTicks::Now();

  if (last_point_valid_) {
    float rise = touch_point.y() - last_point_.y();
    float run = touch_point.x() - last_point_.x();
    float magnitude = sqrt(rise * rise + run * run);
    if (magnitude >= kMinSegmentLength) {
      animations_.push_back(make_scoped_ptr(new MotionDecayAnimation(
          current_ticks, kAnimationDurationMs, last_point_, touch_point)));
      last_point_ = touch_point;
    }
  } else {
    last_point_ = touch_point;
  }
  last_point_valid_ = last_action_ != mojo::EVENT_TYPE_POINTER_UP;

  if (last_action_ == mojo::EVENT_TYPE_POINTER_UP ||
      last_action_ == mojo::EVENT_TYPE_POINTER_DOWN) {
    animations_.push_back(make_scoped_ptr(new MaterialSplashAnimation(
        current_ticks, kAnimationDurationMs, touch_point)));
  }
}

// mojo::TextureUploader::Client implementation.
void ViewObserverDelegate::OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) {
  view_->SetSurfaceId(surface_id.Pass());
}

void ViewObserverDelegate::DrawKeysToCanvas(const mojo::Size& size,
                                            SkCanvas* canvas) {
  key_layout_.SetSize(size);
  key_layout_.Draw(canvas);
}

void ViewObserverDelegate::DrawState() {
  base::TimeTicks current_ticks = base::TimeTicks::Now();
  mojo::Size size = ToSize(view_->bounds());
  mojo::GaneshContext::Scope scope(gr_context_.get());
  mojo::GaneshSurface surface(
      gr_context_.get(),
      make_scoped_ptr(new mojo::GLTexture(gl_context_, size)));

  gr_context_.get()->gr()->resetContext(kTextureBinding_GrGLBackendState);

  SkCanvas* canvas = surface.canvas();
  DrawKeysToCanvas(size, canvas);
  DrawAnimations(canvas, current_ticks);
  DrawFloatingKey(canvas, size);

  canvas->flush();
  texture_uploader_->Upload(surface.TakeTexture());
}

void ViewObserverDelegate::DrawAnimations(
    SkCanvas* canvas,
    const base::TimeTicks& current_ticks) {
  auto first_valid_animation = animations_.end();
  for (auto it = animations_.begin(); it != animations_.end(); ++it) {
    bool done = (*it)->IsDone(current_ticks);
    if (!done) {
      (*it)->Draw(canvas, current_ticks);
      if (first_valid_animation == animations_.end()) {
        first_valid_animation = it;
      }
    }
  }

  // Clean up 'done' animations.
  animations_.erase(animations_.begin(), first_valid_animation);
}

void ViewObserverDelegate::DrawFloatingKey(SkCanvas* canvas,
                                           const mojo::Size& size) {
  if (last_key_ && last_point_valid_) {
    SkPaint paint;
    paint.setColor(SK_ColorYELLOW);
    float row_height = static_cast<float>(size.height) / 4.0f;
    float floating_key_width = static_cast<float>(size.width) / 7.0f;
    float left = last_point_.x() - (floating_key_width / 2);
    float top = last_point_.y() - (1.5f * row_height);
    SkRect rect = SkRect::MakeLTRB(left, top, left + floating_key_width,
                                   top + row_height);
    canvas->drawRect(rect, paint);

    skia::RefPtr<SkTypeface> typeface = skia::AdoptRef(
        SkTypeface::CreateFromName("Arial", SkTypeface::kNormal));
    SkPaint text_paint;
    text_paint.setTypeface(typeface.get());
    text_paint.setColor(SK_ColorBLACK);
    text_paint.setTextSize(row_height / 1.7f);
    text_paint.setAntiAlias(true);
    text_paint.setTextAlign(SkPaint::kCenter_Align);
    last_key_->Draw(canvas, text_paint,
                    gfx::RectF(left, top, floating_key_width, row_height));
  }
}

void ViewObserverDelegate::OnFrameComplete() {
  static base::TimeTicks last_time_ticks = base::TimeTicks::Now();
  base::TimeTicks current_ticks = base::TimeTicks::Now();
  if (!animations_.empty()) {
    base::TimeDelta delta = current_ticks - last_time_ticks;
    if (delta > kMinFramePeriodMs) {
      last_time_ticks = current_ticks;
      DrawState();
    } else {
      base::MessageLoop::current()->PostDelayedTask(
          FROM_HERE, base::Bind(&ViewObserverDelegate::OnFrameComplete,
                                weak_factory_.GetWeakPtr()),
          kMinFramePeriodMs - delta);
    }
  }
}

// mojo::ViewObserver implementation.
void ViewObserverDelegate::OnViewBoundsChanged(mojo::View* view,
                                               const mojo::Rect& old_bounds,
                                               const mojo::Rect& new_bounds) {
  DrawState();
}

void ViewObserverDelegate::OnViewInputEvent(mojo::View* view,
                                            const mojo::EventPtr& event) {
  if (event->pointer_data) {
    gfx::Point point(event->pointer_data->x, event->pointer_data->y);
    last_key_ = key_layout_.GetKeyAtPoint(point);
    last_action_ = event->action;
    if (last_action_ == mojo::EVENT_TYPE_POINTER_UP) {
      key_layout_.OnTouchUp(point);
    }
    UpdateState(point);
    DrawState();
  }
}

void ViewObserverDelegate::OnViewDestroyed(mojo::View* view) {
  if (view_ == view) {
    view_->RemoveObserver(this);
    view_ = nullptr;
    gl_context_->Destroy();
  }
}

}  // namespace keyboard
