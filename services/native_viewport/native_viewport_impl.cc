// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/native_viewport_impl.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/time/time.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "services/gles2/gpu_state.h"
#include "services/native_viewport/platform_viewport_headless.h"
#include "ui/events/event.h"

namespace native_viewport {
namespace {

bool IsRateLimitedEventType(ui::Event* event) {
  return event->type() == ui::ET_MOUSE_MOVED ||
         event->type() == ui::ET_MOUSE_DRAGGED ||
         event->type() == ui::ET_TOUCH_MOVED;
}

}  // namespace

NativeViewportImpl::NativeViewportImpl(
    bool is_headless,
    const scoped_refptr<gles2::GpuState>& gpu_state,
    mojo::InterfaceRequest<mojo::NativeViewport> request)
    : is_headless_(is_headless),
      context_provider_(gpu_state),
      sent_metrics_(false),
      metrics_(mojo::ViewportMetrics::New()),
      waiting_for_event_ack_(false),
      binding_(this, request.Pass()),
      weak_factory_(this) {
  binding_.set_error_handler(this);
}

NativeViewportImpl::~NativeViewportImpl() {
  // Destroy the NativeViewport early on as it may call us back during
  // destruction and we want to be in a known state.
  platform_viewport_.reset();
}

void NativeViewportImpl::Create(mojo::SizePtr size,
                                const CreateCallback& callback) {
  create_callback_ = callback;
  metrics_->size = size.Clone();
  if (is_headless_)
    platform_viewport_ = PlatformViewportHeadless::Create(this);
  else
    platform_viewport_ = PlatformViewport::Create(this);
  platform_viewport_->Init(gfx::Rect(size.To<gfx::Size>()));
}

void NativeViewportImpl::RequestMetrics(
    const RequestMetricsCallback& callback) {
  if (!sent_metrics_) {
    callback.Run(metrics_.Clone());
    sent_metrics_ = true;
    return;
  }
  metrics_callback_ = callback;
}

void NativeViewportImpl::Show() {
  platform_viewport_->Show();
}

void NativeViewportImpl::Hide() {
  platform_viewport_->Hide();
}

void NativeViewportImpl::Close() {
  DCHECK(platform_viewport_);
  platform_viewport_->Close();
}

void NativeViewportImpl::SetSize(mojo::SizePtr size) {
  platform_viewport_->SetBounds(gfx::Rect(size.To<gfx::Size>()));
}

void NativeViewportImpl::GetContextProvider(
    mojo::InterfaceRequest<mojo::ContextProvider> request) {
  context_provider_.Bind(request.Pass());
}

void NativeViewportImpl::SetEventDispatcher(
    mojo::NativeViewportEventDispatcherPtr dispatcher) {
  event_dispatcher_ = dispatcher.Pass();
}

void NativeViewportImpl::OnMetricsChanged(mojo::ViewportMetricsPtr metrics) {
  if (metrics_->Equals(*metrics))
    return;

  metrics_ = metrics.Pass();
  sent_metrics_ = false;

  if (!metrics_callback_.is_null()) {
    metrics_callback_.Run(metrics_.Clone());
    metrics_callback_.reset();
    sent_metrics_ = true;
  }
}

void NativeViewportImpl::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget) {
  context_provider_.SetAcceleratedWidget(widget);
  // TODO: The metrics here might not match the actual window size on android
  // where we don't know the actual size until the first OnMetricsChanged call.
  create_callback_.Run(metrics_.Clone());
  sent_metrics_ = true;
  create_callback_.reset();
}

bool NativeViewportImpl::OnEvent(ui::Event* ui_event) {
  if (!event_dispatcher_.get())
    return false;

  // Must not return early before updating capture.
  switch (ui_event->type()) {
    case ui::ET_MOUSE_PRESSED:
    case ui::ET_TOUCH_PRESSED:
      platform_viewport_->SetCapture();
      break;
    case ui::ET_MOUSE_RELEASED:
    case ui::ET_TOUCH_RELEASED:
      platform_viewport_->ReleaseCapture();
      break;
    default:
      break;
  }

  if (waiting_for_event_ack_ && IsRateLimitedEventType(ui_event))
    return false;

  event_dispatcher_->OnEvent(
      mojo::Event::From(*ui_event),
      base::Bind(&NativeViewportImpl::AckEvent, weak_factory_.GetWeakPtr()));
  waiting_for_event_ack_ = true;
  return false;
}

void NativeViewportImpl::OnDestroyed() {
  // This will signal a connection error and cause us to delete |this|.
  binding_.Close();
}

void NativeViewportImpl::OnConnectionError() {
  binding_.set_error_handler(nullptr);
  delete this;
}

void NativeViewportImpl::AckEvent() {
  waiting_for_event_ack_ = false;
}

}  // namespace native_viewport
