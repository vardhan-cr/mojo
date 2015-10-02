// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "services/keyboard_native/view_observer_delegate.h"

#include "base/bind.h"
#include "base/sys_info.h"
#include "mojo/gpu/gl_texture.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/prediction/public/interfaces/prediction.mojom.h"
#include "mojo/skia/ganesh_surface.h"
#include "services/keyboard_native/clip_animation.h"
#include "services/keyboard_native/keyboard_service_impl.h"
#include "services/keyboard_native/predictor.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkTypeface.h"
#include "ui/gfx/geometry/rect_f.h"
#include "ui/gfx/shadow_value.h"
#include "ui/gfx/skia_util.h"
#include "ui/gfx/vector2d.h"

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
      predictor_(nullptr),
      view_(nullptr),
      id_namespace_(0u),
      surface_id_(1u),
      key_layout_(),
      needs_draw_(false),
      frame_pending_(false),
      floating_key_height_(0.0f),
      floating_key_width_(0.0f),
      weak_factory_(this) {
  key_layout_.SetTextCallback(
      base::Bind(&ViewObserverDelegate::OnText, weak_factory_.GetWeakPtr()));
  key_layout_.SetDeleteCallback(
      base::Bind(&ViewObserverDelegate::OnDelete, weak_factory_.GetWeakPtr()));
  key_layout_.SetSuggestTextCallback(base::Bind(
      &ViewObserverDelegate::OnSuggestText, weak_factory_.GetWeakPtr()));
  submit_frame_callback_ = base::Bind(&ViewObserverDelegate::OnFrameComplete,
                                      weak_factory_.GetWeakPtr());
}

ViewObserverDelegate::~ViewObserverDelegate() {
}

void ViewObserverDelegate::SetKeyboardServiceImpl(
    KeyboardServiceImpl* keyboard_service_impl) {
  keyboard_service_impl_ = keyboard_service_impl;
}

void ViewObserverDelegate::SetIdNamespace(uint32_t id_namespace) {
  id_namespace_ = id_namespace;
  UpdateSurfaceIds();
}

void ViewObserverDelegate::UpdateSurfaceIds() {
  auto surface_id = mojo::SurfaceId::New();
  surface_id->id_namespace = id_namespace_;
  surface_id->local = surface_id_;
  view_->SetSurfaceId(surface_id.Pass());
}

void ViewObserverDelegate::OnViewCreated(mojo::View* view, mojo::Shell* shell) {
  if (view_ != nullptr) {
    view_->RemoveObserver(this);
  }
  view_ = view;
  view_->AddObserver(this);
  gl_context_ = mojo::GLContext::Create(shell);
  mojo::ResourceReturnerPtr resource_returner;
  texture_cache_.reset(new mojo::TextureCache(gl_context_, &resource_returner));
  gr_context_.reset(new mojo::GaneshContext(gl_context_));
  mojo::ServiceProviderPtr surfaces_service_provider;
  shell->ConnectToApplication("mojo:surfaces_service",
                              mojo::GetProxy(&surfaces_service_provider),
                              nullptr);
  mojo::ConnectToService(surfaces_service_provider.get(), &surface_);
  surface_->SetResourceReturner(resource_returner.Pass());
  surface_->CreateSurface(surface_id_);
  surface_->GetIdNamespace(base::Bind(&ViewObserverDelegate::SetIdNamespace,
                                      base::Unretained(this)));
  predictor_ = new Predictor(shell);
  predictor_->SetUpdateCallback(base::Bind(
      &ViewObserverDelegate::OnUpdateSuggestion, weak_factory_.GetWeakPtr()));
  key_layout_.SetPredictor(predictor_);
  IssueDraw();
}

void ViewObserverDelegate::OnText(const std::string& text) {
  keyboard_service_impl_->OnKey(text.c_str());
}

void ViewObserverDelegate::OnDelete() {
  keyboard_service_impl_->OnDelete();
}

void ViewObserverDelegate::OnSuggestText(const std::string& text) {
  std::string text_with_space = text + " ";
  keyboard_service_impl_->OnKey(text_with_space.c_str());
}

void ViewObserverDelegate::OnUpdateSuggestion() {
  IssueDraw();
}

void ViewObserverDelegate::UpdateState(int32 pointer_id,
                                       mojo::EventType action,
                                       const gfx::PointF& touch_point) {
  // Ignore touches outside of key area.
  if (!key_area_.Contains(touch_point)) {
    return;
  }

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
    it2->second->last_point_valid = action != mojo::EventType::POINTER_UP;
  }

  if (action == mojo::EventType::POINTER_UP ||
      action == mojo::EventType::POINTER_DOWN) {
    animations_.push_back(make_scoped_ptr(new MaterialSplashAnimation(
        current_ticks, kAnimationDurationMs, touch_point)));
  }
}

void ViewObserverDelegate::DrawKeysToCanvas(const gfx::RectF& key_area,
                                            SkCanvas* canvas) {
  key_layout_.SetKeyArea(key_area);
  key_layout_.Draw(canvas);
}

void ViewObserverDelegate::DrawState() {
  base::TimeTicks current_ticks = base::TimeTicks::Now();
  mojo::Size size = ToSize(view_->bounds());
  mojo::GaneshContext::Scope scope(gr_context_.get());
  scoped_ptr<mojo::TextureCache::TextureInfo> texture_info(
      texture_cache_->GetTexture(size).Pass());
  mojo::GaneshSurface surface(gr_context_.get(),
                              texture_info->Texture().Pass());

  gr_context_.get()->gr()->resetContext(kTextureBinding_GrGLBackendState);

  SkCanvas* canvas = surface.canvas();

  canvas->clear(SK_ColorTRANSPARENT);

  // If we have a clip animation it must go first as only things drawn after the
  // clip is set will be clipped.
  if (clip_animation_) {
    if (!clip_animation_->IsDone(current_ticks)) {
      clip_animation_->Draw(canvas, current_ticks);
    } else {
      clip_animation_.reset();
    }
  }

  DrawKeysToCanvas(key_area_, canvas);
  // clip animations to the key area.
  canvas->clipRect(SkRect::MakeXYWH(key_area_.x(), key_area_.y(),
                                    key_area_.width(), key_area_.height()));
  DrawAnimations(canvas, current_ticks);

  // reset clip to whatever the clip animation sets.
  canvas->clipRect(SkRect::MakeXYWH(0, 0, size.width, size.height),
                   SkRegion::kReplace_Op);
  if (clip_animation_) {
    if (!clip_animation_->IsDone(current_ticks)) {
      clip_animation_->Draw(canvas, current_ticks);
    } else {
      clip_animation_.reset();
    }
  }

  DrawFloatingKey(canvas, floating_key_height_, floating_key_width_);

  canvas->flush();
  scoped_ptr<mojo::GLTexture> surface_texture(surface.TakeTexture().Pass());
  mojo::FramePtr frame = mojo::TextureUploader::GetUploadFrame(
      gl_context_, texture_info->ResourceId(), surface_texture);
  texture_cache_->NotifyPendingResourceReturn(texture_info->ResourceId(),
                                              surface_texture.Pass());
  surface_->SubmitFrame(surface_id_, frame.Pass(), submit_frame_callback_);
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
                                           float floating_key_height,
                                           float floating_key_width) {
  if (active_pointer_ids_.empty()) {
    return;
  }

  for (auto& pointer_state : active_pointer_state_) {
    if (pointer_state.second->last_key != nullptr &&
        pointer_state.second->last_point_valid) {
      SkPaint paint;
      paint.setColor(SK_ColorYELLOW);
      float left =
          pointer_state.second->last_point.x() - (floating_key_width / 2);
      float top =
          pointer_state.second->last_point.y() - (0.45f * key_area_.height());
      SkRect rect = SkRect::MakeLTRB(left, top, left + floating_key_width,
                                     top + floating_key_height);

      // Add shadow beneath the floating key.
      paint.setStrokeJoin(SkPaint::kRound_Join);
      int blur = 20;
      float ratio_x_from_center =
          (left + (floating_key_width / 2) - (key_area_.width() / 2)) /
          (key_area_.width() / 2);
      float ratio_y_from_center =
          (top + (floating_key_height / 2) - (key_area_.height() / 2)) /
          (key_area_.height() / 2);
      SkColor color = SkColorSetARGB(0x80, 0, 0, 0);
      std::vector<gfx::ShadowValue> shadows;
      shadows.push_back(gfx::ShadowValue(
          gfx::Vector2d(ratio_x_from_center * 10, ratio_y_from_center * 10),
          blur, color));
      skia::RefPtr<SkDrawLooper> looper = gfx::CreateShadowDrawLooper(shadows);
      paint.setLooper(looper.get());

      canvas->drawRect(rect, paint);

      skia::RefPtr<SkTypeface> typeface = skia::AdoptRef(
          SkTypeface::CreateFromName("Arial", SkTypeface::kNormal));
      SkPaint text_paint;
      text_paint.setTypeface(typeface.get());
      text_paint.setColor(SK_ColorBLACK);
      text_paint.setTextSize(floating_key_height / 1.7f);
      text_paint.setAntiAlias(true);
      text_paint.setTextAlign(SkPaint::kCenter_Align);
      pointer_state.second->last_key->Draw(
          canvas, text_paint,
          gfx::RectF(left, top, floating_key_width, floating_key_height));
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
  surface_->DestroySurface(surface_id_);
  surface_id_++;
  surface_->CreateSurface(surface_id_);
  UpdateSurfaceIds();

  floating_key_height_ = static_cast<float>(new_bounds.height) / 5.0f;
  floating_key_width_ = static_cast<float>(new_bounds.width) / 8.0f;

  // One third of our height will be used for overlapping views behind us.  Only
  // the floating key will be drawn into that area.  The remaining two thirds
  // will be the key area.
  float overlap = new_bounds.height / 3.0f;
  key_area_ =
      gfx::RectF(0, overlap, new_bounds.width, new_bounds.height - overlap);

  // Upon resize, kick off a clip animation centered on our key area with a
  // radius equal to the length from a corner of the new bounds to the center of
  // the key area.
  base::TimeTicks current_ticks = base::TimeTicks::Now();
  gfx::PointF center(key_area_.width() / 2.0f,
                     overlap + (key_area_.height() / 2.0f));
  float radius = sqrt(new_bounds.width / 2.0f * new_bounds.width / 2.0f +
                      new_bounds.height / 2.0f * new_bounds.height / 2.0f);
  clip_animation_.reset(
      new ClipAnimation(current_ticks, kAnimationDurationMs, center, radius));

  IssueDraw();
}

void ViewObserverDelegate::OnViewInputEvent(mojo::View* view,
                                            const mojo::EventPtr& event) {
  if (event->pointer_data) {
    gfx::PointF point(event->pointer_data->x, event->pointer_data->y);

    if (event->action == mojo::EventType::POINTER_UP) {
      key_layout_.OnTouchUp(point);
    }

    if (event->action == mojo::EventType::POINTER_DOWN ||
        event->action == mojo::EventType::POINTER_UP) {
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
      if (event->action == mojo::EventType::POINTER_DOWN) {
        active_pointer_ids_.push_back(event->pointer_data->pointer_id);
        PointerState* pointer_state = new PointerState();
        pointer_state->last_key = nullptr;
        pointer_state->last_point = gfx::PointF();
        pointer_state->last_point_valid = false;
        active_pointer_state_[event->pointer_data->pointer_id].reset(
            pointer_state);
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
