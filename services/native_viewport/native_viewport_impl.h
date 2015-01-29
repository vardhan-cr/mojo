// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_
#define SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "cc/surfaces/surface_id.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
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

// A NativeViewportImpl is bound to a message pipe and to a PlatformViewport.
// The NativeViewportImpl's lifetime ends when either the message pipe is closed
// or the PlatformViewport informs the NativeViewportImpl that it has been
// destroyed.
class NativeViewportImpl : public mojo::NativeViewport,
                           public PlatformViewport::Delegate,
                           public mojo::ErrorHandler {
  using CreateCallback = mojo::Callback<void(uint64_t native_viewport_id,
                                             mojo::ViewportMetricsPtr metrics)>;
  using MetricsCallback =
      mojo::Callback<void(mojo::ViewportMetricsPtr metrics)>;

 public:
  NativeViewportImpl(mojo::ApplicationImpl* app,
                     bool is_headless,
                     mojo::InterfaceRequest<mojo::NativeViewport> request);
  ~NativeViewportImpl() override;

  // InterfaceImpl<NativeViewport> implementation.
  void Create(mojo::SizePtr size, const CreateCallback& callback) override;
  void RequestMetrics(const MetricsCallback& callback) override;
  void Show() override;
  void Hide() override;
  void Close() override;
  void SetSize(mojo::SizePtr size) override;
  void SubmittedFrame(mojo::SurfaceIdPtr surface_id) override;
  void SetEventDispatcher(
      mojo::NativeViewportEventDispatcherPtr dispatcher) override;

  // PlatformViewport::Delegate implementation.
  void OnMetricsChanged(mojo::ViewportMetricsPtr metrics) override;
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget) override;
  bool OnEvent(ui::Event* ui_event) override;
  void OnDestroyed() override;

  // mojo::ErrorHandler implementation.
  void OnConnectionError() override;

  void AckEvent();

 private:
  bool is_headless_;
  scoped_ptr<PlatformViewport> platform_viewport_;
  scoped_ptr<ViewportSurface> viewport_surface_;
  uint64_t widget_id_;
  bool sent_metrics_;
  mojo::ViewportMetricsPtr metrics_;
  mojo::GpuPtr gpu_service_;
  mojo::SurfacePtr surface_;
  cc::SurfaceId child_surface_id_;
  bool waiting_for_event_ack_;
  CreateCallback create_callback_;
  MetricsCallback metrics_callback_;
  mojo::NativeViewportEventDispatcherPtr event_dispatcher_;
  mojo::Binding<mojo::NativeViewport> binding_;
  base::WeakPtrFactory<NativeViewportImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewportImpl);
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_NATIVE_VIEWPORT_IMPL_H_
