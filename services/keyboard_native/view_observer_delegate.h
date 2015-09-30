// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
#define SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_

#include "base/time/time.h"
#include "mojo/gpu/texture_cache.h"
#include "mojo/gpu/texture_uploader.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"
#include "services/keyboard_native/key_layout.h"
#include "services/keyboard_native/material_splash_animation.h"
#include "services/keyboard_native/motion_decay_animation.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect_f.h"

namespace keyboard {

class KeyboardServiceImpl;
class Predictor;

struct PointerState {
  KeyLayout::Key* last_key;
  gfx::PointF last_point;
  bool last_point_valid;
};

class ViewObserverDelegate : public mojo::ViewObserver {
 public:
  ViewObserverDelegate();
  ~ViewObserverDelegate() override;
  void SetKeyboardServiceImpl(KeyboardServiceImpl* keyboard_service_impl);
  void OnViewCreated(mojo::View* view, mojo::Shell* shell);

 private:
  void SetIdNamespace(uint32_t id_namespace);
  void UpdateSurfaceIds();
  void OnFrameComplete();
  void OnText(const std::string& text);
  void OnDelete();
  void OnSuggestText(const std::string& text);
  void OnUpdateSuggestion();
  void DrawState();
  void DrawKeysToCanvas(const gfx::RectF& key_area, SkCanvas* canvas);
  void DrawAnimations(SkCanvas* canvas, const base::TimeTicks& current_ticks);
  void DrawFloatingKey(SkCanvas* canvas,
                       float floating_key_height,
                       float floating_key_width);
  void UpdateState(int32 pointer_id,
                   mojo::EventType action,
                   const gfx::PointF& touch_point);
  void IssueDraw();

  // mojo::ViewObserver implementation.
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

  KeyboardServiceImpl* keyboard_service_impl_;
  Predictor* predictor_;
  mojo::View* view_;
  uint32_t id_namespace_;
  uint32_t surface_id_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  scoped_ptr<mojo::GaneshContext> gr_context_;
  scoped_ptr<mojo::TextureCache> texture_cache_;
  scoped_ptr<Animation> clip_animation_;
  std::deque<scoped_ptr<Animation>> animations_;
  KeyLayout key_layout_;
  std::vector<int32> active_pointer_ids_;
  std::map<int32, scoped_ptr<PointerState>> active_pointer_state_;
  bool needs_draw_;
  bool frame_pending_;
  float floating_key_height_;
  float floating_key_width_;
  gfx::RectF key_area_;
  mojo::SurfacePtr surface_;
  base::Closure submit_frame_callback_;
  base::WeakPtrFactory<ViewObserverDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewObserverDelegate);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
