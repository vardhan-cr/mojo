// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
#define SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_

#include "base/time/time.h"
#include "mojo/gpu/texture_uploader.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"
#include "services/keyboard_native/key_layout.h"
#include "services/keyboard_native/material_splash_animation.h"
#include "services/keyboard_native/motion_decay_animation.h"
#include "ui/gfx/geometry/point.h"

namespace keyboard {

class KeyboardServiceImpl;

struct PointerState {
  KeyLayout::Key* last_key;
  gfx::Point last_point;
  bool last_point_valid;
};

class ViewObserverDelegate : public mojo::ViewObserver,
                             public mojo::TextureUploader::Client {
 public:
  ViewObserverDelegate();
  ~ViewObserverDelegate() override;
  void SetKeyboardServiceImpl(KeyboardServiceImpl* keyboard_service_impl);
  void OnViewCreated(mojo::View* view, mojo::Shell* shell);

 private:
  void OnText(const std::string& text);
  void DrawState();
  void DrawKeysToCanvas(const mojo::Size& size, SkCanvas* canvas);
  void DrawAnimations(SkCanvas* canvas, const base::TimeTicks& current_ticks);
  void DrawFloatingKey(SkCanvas* canvas, const mojo::Size& size);
  void UpdateState(int32 pointer_id, int action, const gfx::Point& touch_point);
  void IssueDraw();

  // mojo::TextureUploader::Client implementation.
  void OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) override;
  void OnFrameComplete() override;

  // mojo::ViewObserver implementation.
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

  KeyboardServiceImpl* keyboard_service_impl_;
  mojo::View* view_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  scoped_ptr<mojo::GaneshContext> gr_context_;
  scoped_ptr<mojo::TextureUploader> texture_uploader_;
  std::deque<scoped_ptr<Animation>> animations_;
  KeyLayout key_layout_;
  std::vector<int32> active_pointer_ids_;
  std::map<int32, scoped_ptr<PointerState>> active_pointer_state_;
  bool needs_draw_;
  bool frame_pending_;
  base::WeakPtrFactory<ViewObserverDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewObserverDelegate);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
