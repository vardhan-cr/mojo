// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string>

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "examples/spinning_cube/gles2_client_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"

namespace examples {

class SpinningCubeApp : public mojo::ApplicationDelegate,
                        public mojo::NativeViewportEventDispatcher,
                        public mojo::ErrorHandler {
 public:
  SpinningCubeApp() : dispatcher_binding_(this) {}

  ~SpinningCubeApp() override {
    // TODO(darin): Fix shutdown so we don't need to leak this.
    mojo_ignore_result(gles2_client_.release());
  }

  void Initialize(mojo::ApplicationImpl* app) override {
    app->ConnectToService("mojo:native_viewport_service", &viewport_);
    viewport_.set_error_handler(this);

    SetEventDispatcher();

    // TODO(jamesr): Should be mojo:gpu_service
    app->ConnectToService("mojo:native_viewport_service", &gpu_service_);

    mojo::SizePtr size(mojo::Size::New());
    size->width = 800;
    size->height = 600;
    viewport_->Create(size.Pass(),
                      base::Bind(&SpinningCubeApp::OnCreatedNativeViewport,
                                 base::Unretained(this)));
    viewport_->Show();
  }

  void OnMetricsChanged(mojo::ViewportMetricsPtr metrics) {
    assert(metrics);
    if (gles2_client_)
      gles2_client_->SetSize(*metrics->size);
    viewport_->RequestMetrics(
        base::Bind(&SpinningCubeApp::OnMetricsChanged, base::Unretained(this)));
  }

  void OnEvent(mojo::EventPtr event,
               const mojo::Callback<void()>& callback) override {
    assert(event);
    if (event->location_data && event->location_data->in_view_location)
      gles2_client_->HandleInputEvent(*event);
    callback.Run();
  }

 private:
  void SetEventDispatcher() {
    mojo::NativeViewportEventDispatcherPtr ptr;
    dispatcher_binding_.Bind(GetProxy(&ptr));
    viewport_->SetEventDispatcher(ptr.Pass());
  }

  void OnCreatedNativeViewport(uint64_t native_viewport_id,
                               mojo::ViewportMetricsPtr metrics) {
    mojo::ViewportParameterListenerPtr listener;
    mojo::CommandBufferPtr command_buffer;
    // TODO(jamesr): Output to a surface instead.
    gpu_service_->CreateOnscreenGLES2Context(
        native_viewport_id, metrics->size.Clone(), GetProxy(&command_buffer),
        listener.Pass());
    gles2_client_.reset(new GLES2ClientImpl(command_buffer.Pass()));
    gles2_client_->SetSize(*metrics->size);
    viewport_->RequestMetrics(
        base::Bind(&SpinningCubeApp::OnMetricsChanged, base::Unretained(this)));
  }

  // ErrorHandler implementation.
  void OnConnectionError() override { mojo::RunLoop::current()->Quit(); }

  scoped_ptr<GLES2ClientImpl> gles2_client_;
  mojo::NativeViewportPtr viewport_;
  mojo::GpuPtr gpu_service_;
  mojo::Binding<NativeViewportEventDispatcher> dispatcher_binding_;

  DISALLOW_COPY_AND_ASSIGN(SpinningCubeApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunner runner(new examples::SpinningCubeApp);
  return runner.Run(shell_handle);
}
