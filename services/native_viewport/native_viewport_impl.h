// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_
#define SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/surfaces/surface_id.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces_service.mojom.h"
#include "services/native_viewport/platform_viewport.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {
class Event;
}

namespace mojo {
class ApplicationImpl;
}

namespace native_viewport {
class ViewportSurface;

class NativeViewportImpl : public mojo::InterfaceImpl<mojo::NativeViewport>,
                           public PlatformViewport::Delegate {
 public:
  NativeViewportImpl(mojo::ApplicationImpl* app, bool is_headless);
  ~NativeViewportImpl() override;

  // InterfaceImpl<NativeViewport> implementation.
  void Create(mojo::SizePtr size,
              const mojo::Callback<void(uint64_t)>& callback) override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void SetSize(mojo::SizePtr size) override;
  void SubmittedFrame(mojo::SurfaceIdPtr surface_id) override;
  void SetEventDispatcher(
      mojo::NativeViewportEventDispatcherPtr dispatcher) override;

  // PlatformViewport::Delegate implementation.
  void OnBoundsChanged(const gfx::Rect& bounds) override;
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget) override;
  bool OnEvent(ui::Event* ui_event) override;
  void OnDestroyed() override;

  void AckEvent();

 private:
  void ProcessOnBoundsChanged();

  bool is_headless_;
  scoped_ptr<PlatformViewport> platform_viewport_;
  scoped_ptr<ViewportSurface> viewport_surface_;
  uint64_t widget_id_;
  gfx::Size size_;
  mojo::GpuPtr gpu_service_;
  mojo::SurfacesServicePtr surfaces_service_;
  cc::SurfaceId child_surface_id_;
  bool waiting_for_event_ack_;
  mojo::Callback<void(uint64_t)> create_callback_;
  mojo::NativeViewportEventDispatcherPtr event_dispatcher_;
  base::WeakPtrFactory<NativeViewportImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewportImpl);
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_
