// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
#define SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_

#include <memory>

#include "mojo/gpu/texture_uploader.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"
#include "services/keyboard_native/key_layout.h"
#include "ui/gfx/geometry/point.h"

namespace keyboard {

class KeyboardServiceImpl;

class ViewObserverDelegate : public mojo::ViewObserver,
                             public mojo::TextureUploader::Client {
 public:
  ViewObserverDelegate();
  ~ViewObserverDelegate() override;
  void SetKeyboardServiceImpl(KeyboardServiceImpl* keyboard_service_impl);
  void OnViewCreated(mojo::View* view, mojo::Shell* shell);

 private:
  void OnText(const std::string& text);
  void DrawKeys(const mojo::Size& size);
  void DrawKeysToCanvas(const mojo::Size& size, SkCanvas* canvas);
  void DrawMovePointTrail(SkCanvas* canvas);
  void DrawFloatingKey(SkCanvas* canvas,
                       const mojo::Size& size,
                       float current_touch_x,
                       float current_touch_y);
  void UpdateMovePoints(mojo::Size size, const mojo::EventPtr& event);

  // mojo::TextureUploader::Client implementation.
  void OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) override;

  // mojo::ViewObserver implementation.
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

  KeyboardServiceImpl* keyboard_service_impl_;
  mojo::View* view_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  std::unique_ptr<mojo::GaneshContext> gr_context_;
  std::unique_ptr<mojo::TextureUploader> texture_uploader_;
  std::deque<gfx::Point> move_points_;
  int last_action_;
  KeyLayout key_layout_;
  KeyLayout::Key* last_key_;
  base::WeakPtrFactory<ViewObserverDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewObserverDelegate);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_VIEW_OBSERVER_DELEGATE_H_
