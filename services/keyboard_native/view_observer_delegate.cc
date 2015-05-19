// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/keyboard_native/view_observer_delegate.h"

#include "base/bind.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/skia/ganesh_surface.h"
#include "services/keyboard_native/keyboard_service_impl.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace keyboard {

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
  gr_context_ = std::unique_ptr<mojo::GaneshContext>(
      new mojo::GaneshContext(gl_context_));
  texture_uploader_ = std::unique_ptr<mojo::TextureUploader>(
      new mojo::TextureUploader(this, shell, gl_context_));

  DrawKeys(ToSize(view_->bounds()));
}

void ViewObserverDelegate::OnText(const std::string& text) {
  // TODO: call keyboard service
}

void ViewObserverDelegate::DrawKeysToCanvas(const mojo::Size& size,
                                            SkCanvas* canvas) {
  key_layout_.SetSize(size);
  key_layout_.Draw(canvas);
}

void ViewObserverDelegate::DrawKeys(const mojo::Size& size) {
  mojo::GaneshContext::Scope scope(gr_context_.get());
  mojo::GaneshSurface surface(
      gr_context_.get(),
      make_scoped_ptr(new mojo::GLTexture(gl_context_, size)));

  // Need this to reset the texture binding state after setting new texture size
  gr_context_.get()->gr()->resetContext(kTextureBinding_GrGLBackendState);

  SkCanvas* canvas = surface.canvas();

  DrawKeysToCanvas(size, canvas);

  canvas->flush();

  texture_uploader_->Upload(surface.TakeTexture());
}

void ViewObserverDelegate::UpdateMovePoints(mojo::Size size,
                                            const mojo::EventPtr& event) {
  static const size_t kMaxSize = 30;

  switch (event->action) {
    case mojo::EVENT_TYPE_POINTER_UP:
      // commit point
      move_points_.clear();
      break;
    case mojo::EVENT_TYPE_POINTER_DOWN:
    case mojo::EVENT_TYPE_POINTER_MOVE:
      if (event->pointer_data) {
        move_points_.push_back(
            gfx::Point(event->pointer_data->x, event->pointer_data->y));
        if (move_points_.size() > kMaxSize) {
          // We need to pop in sets of three as the tail of the 'line' drawn
          // with these move points is being drawn as a cubic path.  Popping
          // more or less than three results in the line oscillating as the path
          // goes through points in a different way than was drawn before.
          move_points_.pop_front();
          move_points_.pop_front();
          move_points_.pop_front();
        }
      }
      break;
    case mojo::EVENT_TYPE_POINTER_CANCEL:
    case mojo::EVENT_TYPE_KEY_RELEASED:
    case mojo::EVENT_TYPE_KEY_PRESSED:
    case mojo::EVENT_TYPE_UNKNOWN:
    default:
      break;
  }

  // draw line
  mojo::GaneshContext::Scope scope(gr_context_.get());
  mojo::GaneshSurface surface(
      gr_context_.get(),
      make_scoped_ptr(new mojo::GLTexture(gl_context_, size)));

  gr_context_.get()->gr()->resetContext(kTextureBinding_GrGLBackendState);

  SkCanvas* canvas = surface.canvas();
  DrawKeysToCanvas(size, canvas);

  if (move_points_.size() > 1) {
    DrawMovePointTrail(canvas);
  }

  if (last_key_ != nullptr && event->action != mojo::EVENT_TYPE_POINTER_UP) {
    DrawFloatingKey(canvas, size, event->pointer_data->x,
                    event->pointer_data->y);
  }

  canvas->flush();
  texture_uploader_->Upload(surface.TakeTexture());
}

void ViewObserverDelegate::DrawMovePointTrail(SkCanvas* canvas) {
  std::deque<gfx::Point>::reverse_iterator it = move_points_.rbegin();
  if (it != move_points_.rend()) {
    SkPaint paint;
    paint.setColor(SK_ColorBLUE);
    paint.setStrokeWidth(15);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeCap(SkPaint::kRound_Cap);
    paint.setStrokeJoin(SkPaint::kRound_Join);

    SkPath path;
    path.moveTo(static_cast<float>(it->x()), static_cast<float>(it->y()));
    it++;
    int extraPoints = ((move_points_.size() - 1) % 3);
    switch (extraPoints) {
      case 1: {
        gfx::Point point1 = *it;
        it++;
        path.lineTo(static_cast<float>(point1.x()),
                    static_cast<float>(point1.y()));
      } break;
      case 2: {
        gfx::Point point1 = *it;
        it++;
        gfx::Point point2 = *it;
        it++;
        path.quadTo(
            static_cast<float>(point1.x()), static_cast<float>(point1.y()),
            static_cast<float>(point2.x()), static_cast<float>(point2.y()));
      } break;
    }
    // Remaining points are a multiple of 3 so we don't need to check for end
    // each it++
    while (it != move_points_.rend()) {
      gfx::Point point1 = *it;
      it++;
      gfx::Point point2 = *it;
      it++;
      gfx::Point point3 = *it;
      it++;
      path.cubicTo(
          static_cast<float>(point1.x()), static_cast<float>(point1.y()),
          static_cast<float>(point2.x()), static_cast<float>(point2.y()),
          static_cast<float>(point3.x()), static_cast<float>(point3.y()));
    }

    canvas->drawPath(path, paint);
  }
}

void ViewObserverDelegate::DrawFloatingKey(SkCanvas* canvas,
                                           const mojo::Size& size,
                                           float current_touch_x,
                                           float current_touch_y) {
  SkPaint paint;
  paint.setColor(SK_ColorYELLOW);
  float row_height = static_cast<float>(size.height) / 4.0f;
  float floating_key_width = static_cast<float>(size.width) / 7.0f;
  float left = current_touch_x - (floating_key_width / 2);
  float top = current_touch_y - (1.5f * row_height);
  SkRect rect =
      SkRect::MakeLTRB(left, top, left + floating_key_width, top + row_height);
  canvas->drawRect(rect, paint);

  skia::RefPtr<SkTypeface> typeface =
      skia::AdoptRef(SkTypeface::CreateFromName("Arial", SkTypeface::kNormal));
  SkPaint text_paint;
  text_paint.setTypeface(typeface.get());
  text_paint.setColor(SK_ColorBLACK);
  text_paint.setTextSize(row_height / 1.7f);
  text_paint.setAntiAlias(true);
  text_paint.setTextAlign(SkPaint::kCenter_Align);
  last_key_->Draw(canvas, text_paint,
                  gfx::RectF(left, top, floating_key_width, row_height));
}

// mojo::TextureUploader::Client implementation.
void ViewObserverDelegate::OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) {
  view_->SetSurfaceId(surface_id.Pass());
}

// mojo::ViewObserver implementation.
void ViewObserverDelegate::OnViewBoundsChanged(mojo::View* view,
                                               const mojo::Rect& old_bounds,
                                               const mojo::Rect& new_bounds) {
  DrawKeys(ToSize(new_bounds));
}

void ViewObserverDelegate::OnViewInputEvent(mojo::View* view,
                                            const mojo::EventPtr& event) {
  if (keyboard_service_impl_) {
    keyboard_service_impl_->OnViewInputEvent(view, event);
  }
  mojo::Size size = ToSize(view_->bounds());
  key_layout_.SetSize(size);

  if (event->pointer_data) {
    gfx::Point point(event->pointer_data->x, event->pointer_data->y);
    last_key_ = key_layout_.GetKeyAtPoint(point);
    last_action_ = event->action;
    if (event->action == mojo::EVENT_TYPE_POINTER_UP) {
      key_layout_.OnTouchUp(point);
    }
    UpdateMovePoints(size, event);
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
