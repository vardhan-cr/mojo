// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "services/keyboard_native/view_observer_delegate.h"

#include "base/bind.h"
#include "base/sys_info.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/skia/ganesh_surface.h"
#include "services/keyboard_native/clip_animation.h"
#include "services/keyboard_native/keyboard_service_impl.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/geometry/rect_f.h"

namespace keyboard {

static const base::TimeDelta kAnimationDurationMs =
    base::TimeDelta::FromMilliseconds(1000);

mojo::Size ToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

ViewObserverDelegate::ViewObserverDelegate()
    : keyboard_service_impl_(nullptr),
      view_(nullptr),
      key_layout_(),
      needs_draw_(false),
      frame_pending_(false),
      weak_factory_(this) {
  key_layout_.SetTextCallback(
      base::Bind(&ViewObserverDelegate::OnText, weak_factory_.GetWeakPtr()));
  key_layout_.SetDeleteCallback(
      base::Bind(&ViewObserverDelegate::OnDelete, weak_factory_.GetWeakPtr()));
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

  IssueDraw();
}

void ViewObserverDelegate::OnText(const std::string& text) {
  keyboard_service_impl_->OnKey(text.c_str());
}

void ViewObserverDelegate::OnDelete() {
  keyboard_service_impl_->OnDelete();
}

void ViewObserverDelegate::UpdateState(int32 pointer_id,
                                       int action,
                                       const gfx::Point& touch_point) {
  base::TimeTicks current_ticks = base::TimeTicks::Now();

  auto it2 = active_pointer_state_.find(pointer_id);
  if (it2 != active_pointer_state_.end()) {
    if (it2->second->last_point_valid) {
      animations_.push_back(make_scoped_ptr(
          new MotionDecayAnimation(current_ticks, kAnimationDurationMs,
                                   it2->second->last_point, touch_point)));
      it2->second->last_point = touch_point;
    } else {
      it2->second->last_point = touch_point;
    }
    it2->second->last_point_valid = action != mojo::EVENT_TYPE_POINTER_UP;
  }

  if (action == mojo::EVENT_TYPE_POINTER_UP ||
      action == mojo::EVENT_TYPE_POINTER_DOWN) {
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

  // If we have a clip animation it must go first as only things drawn after the
  // clip is set will be clipped.
  if (clip_animation_) {
    if (!clip_animation_->IsDone(current_ticks)) {
      clip_animation_->Draw(canvas, current_ticks);
    } else {
      clip_animation_.reset();
    }
  }
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
  if (active_pointer_ids_.empty()) {
    return;
  }

  for (auto& pointer_state : active_pointer_state_) {
    if (pointer_state.second->last_key != nullptr &&
        pointer_state.second->last_point_valid) {
      SkPaint paint;
      paint.setColor(SK_ColorYELLOW);
      float row_height = static_cast<float>(size.height) / 4.0f;
      float floating_key_width = static_cast<float>(size.width) / 7.0f;
      float left =
          pointer_state.second->last_point.x() - (floating_key_width / 2);
      float top = pointer_state.second->last_point.y() - (1.5f * row_height);
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
      pointer_state.second->last_key->Draw(
          canvas, text_paint,
          gfx::RectF(left, top, floating_key_width, row_height));
    }
  }
}

void ViewObserverDelegate::IssueDraw() {
  if (!frame_pending_) {
    DrawState();
    frame_pending_ = true;
    needs_draw_ = !animations_.empty() || clip_animation_;
  } else {
    needs_draw_ = true;
  }
}

void ViewObserverDelegate::OnFrameComplete() {
  frame_pending_ = false;
  if (needs_draw_) {
    IssueDraw();
  }
}

// mojo::ViewObserver implementation.
void ViewObserverDelegate::OnViewBoundsChanged(mojo::View* view,
                                               const mojo::Rect& old_bounds,
                                               const mojo::Rect& new_bounds) {
  // Upon resize, kick off a clip animation centered on our new bounds with a
  // radius equal to the length from a corner of to the center of the new
  // bounds.
  base::TimeTicks current_ticks = base::TimeTicks::Now();
  gfx::Point center(new_bounds.width / 2.0f, new_bounds.height / 2.0f);
  float radius = sqrt(new_bounds.width / 2.0f * new_bounds.width / 2.0f +
                      new_bounds.height / 2.0f * new_bounds.height / 2.0f);
  clip_animation_.reset(
      new ClipAnimation(current_ticks, kAnimationDurationMs, center, radius));

  IssueDraw();
}

void ViewObserverDelegate::OnViewInputEvent(mojo::View* view,
                                            const mojo::EventPtr& event) {
  if (event->pointer_data) {
    gfx::Point point(event->pointer_data->x, event->pointer_data->y);

    if (event->action == mojo::EVENT_TYPE_POINTER_UP) {
      key_layout_.OnTouchUp(point);
    }

    if (event->action == mojo::EVENT_TYPE_POINTER_DOWN ||
        event->action == mojo::EVENT_TYPE_POINTER_UP) {
      auto it =
          std::find(active_pointer_ids_.begin(), active_pointer_ids_.end(),
                    event->pointer_data->pointer_id);
      if (it != active_pointer_ids_.end()) {
        active_pointer_ids_.erase(it);
      }

      auto it2 = active_pointer_state_.find(event->pointer_data->pointer_id);
      if (it2 != active_pointer_state_.end()) {
        active_pointer_state_.erase(it2);
      }
      if (event->action == mojo::EVENT_TYPE_POINTER_DOWN) {
        active_pointer_ids_.push_back(event->pointer_data->pointer_id);
        PointerState* pointer_state = new PointerState();
        pointer_state->last_key = nullptr;
        pointer_state->last_point = gfx::Point();
        pointer_state->last_point_valid = false;
        active_pointer_state_[event->pointer_data->pointer_id] =
            make_scoped_ptr(pointer_state);
      }
    }

    auto it2 = active_pointer_state_.find(event->pointer_data->pointer_id);
    if (it2 != active_pointer_state_.end()) {
      active_pointer_state_[event->pointer_data->pointer_id]->last_key =
          key_layout_.GetKeyAtPoint(point);
    }

    UpdateState(event->pointer_data->pointer_id, event->action, point);
    IssueDraw();
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
