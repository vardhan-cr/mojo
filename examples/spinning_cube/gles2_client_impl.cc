// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/spinning_cube/gles2_client_impl.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <math.h>
#include <stdlib.h>

#include "gpu/command_buffer/client/gles2_interface.h"
#include "mojo/public/c/gles2/gles2.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace examples {
namespace {

float CalculateDragDistance(const mojo::Point& start, const mojo::Point& end) {
  return hypot(static_cast<float>(start.x - end.x),
               static_cast<float>(start.y - end.y));
}

float GetRandomColor() {
  return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

}

GLES2ClientImpl::GLES2ClientImpl(mojo::ContextProviderPtr context_provider)
    : last_time_(mojo::GetTimeTicksNow()),
      waiting_to_draw_(false),
      context_provider_(context_provider.Pass()),
      context_(nullptr) {
  context_provider_->Create(nullptr,
                            [this](mojo::CommandBufferPtr command_buffer) {
                              ContextCreated(command_buffer.Pass());
                            });
}

GLES2ClientImpl::~GLES2ClientImpl() {
  MojoGLES2DestroyContext(context_);
}

void GLES2ClientImpl::SetSize(const mojo::Size& size) {
  size_ = size;
  cube_.set_size(size_.width, size_.height);
  if (size_.width == 0 || size_.height == 0 || !context_)
    return;
  static_cast<gpu::gles2::GLES2Interface*>(
      MojoGLES2GetGLES2Interface(context_))->ResizeCHROMIUM(size_.width,
                                                            size_.height,
                                                            1);
  WantToDraw();
}

void GLES2ClientImpl::HandleInputEvent(const mojo::Event& event) {
  switch (event.action) {
  case mojo::EVENT_TYPE_MOUSE_PRESSED:
  case mojo::EVENT_TYPE_TOUCH_PRESSED:
    if (event.flags & mojo::EVENT_FLAGS_RIGHT_MOUSE_BUTTON)
      break;
    capture_point_ = *event.location_data->in_view_location;
    last_drag_point_ = capture_point_;
    drag_start_time_ = mojo::GetTimeTicksNow();
    break;
  case mojo::EVENT_TYPE_MOUSE_DRAGGED:
  case mojo::EVENT_TYPE_TOUCH_MOVED: {
    if (event.flags & mojo::EVENT_FLAGS_RIGHT_MOUSE_BUTTON)
      break;
    int direction =
        (event.location_data->in_view_location->y < last_drag_point_.y ||
         event.location_data->in_view_location->x > last_drag_point_.x)
        ? 1 : -1;
    cube_.set_direction(direction);
    cube_.UpdateForDragDistance(CalculateDragDistance(
        last_drag_point_, *event.location_data->in_view_location));
    WantToDraw();

    last_drag_point_ = *event.location_data->in_view_location;
    break;
  }
  case mojo::EVENT_TYPE_MOUSE_RELEASED:
  case mojo::EVENT_TYPE_TOUCH_RELEASED: {
    if (event.flags & mojo::EVENT_FLAGS_RIGHT_MOUSE_BUTTON) {
      cube_.set_color(GetRandomColor(), GetRandomColor(), GetRandomColor());
      break;
    }
    MojoTimeTicks offset = mojo::GetTimeTicksNow() - drag_start_time_;
    float delta = static_cast<float>(offset) / 1000000.;
    cube_.SetFlingMultiplier(CalculateDragDistance(
        capture_point_, *event.location_data->in_view_location), delta);

    capture_point_ = last_drag_point_ = mojo::Point();
    WantToDraw();
    break;
  }
  default:
    break;
  }
}

void GLES2ClientImpl::ContextCreated(mojo::CommandBufferPtr command_buffer) {
  context_ = MojoGLES2CreateContext(
      command_buffer.PassMessagePipe().release().value(), &ContextLostThunk,
      this, mojo::Environment::GetDefaultAsyncWaiter());
  MojoGLES2MakeCurrent(context_);
  cube_.Init();
  WantToDraw();
}

void GLES2ClientImpl::ContextLost() {
  cube_.OnGLContextLost();
  MojoGLES2DestroyContext(context_);
  context_ = nullptr;
  context_provider_->Create(nullptr,
                            [this](mojo::CommandBufferPtr command_buffer) {
                              ContextCreated(command_buffer.Pass());
                            });
}

void GLES2ClientImpl::ContextLostThunk(void* closure) {
  static_cast<GLES2ClientImpl*>(closure)->ContextLost();
}

void GLES2ClientImpl::WantToDraw() {
  if (waiting_to_draw_ || !context_)
    return;
  waiting_to_draw_ = true;
  mojo::RunLoop::current()->PostDelayedTask([this]() { Draw(); },
                                            MojoTimeTicks(16667));
}

void GLES2ClientImpl::Draw() {
  waiting_to_draw_ = false;
  if (!context_)
    return;
  MojoTimeTicks now = mojo::GetTimeTicksNow();
  MojoTimeTicks offset = now - last_time_;
  float delta = static_cast<float>(offset) / 1000000.;
  last_time_ = now;
  cube_.UpdateForTimeDelta(delta);
  cube_.Draw();

  MojoGLES2SwapBuffers();
  WantToDraw();
}

}  // namespace examples
