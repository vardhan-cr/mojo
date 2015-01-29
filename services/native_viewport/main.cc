// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/native_viewport/public/cpp/args.h"
#include "services/gles2/gpu_impl.h"
#include "services/native_viewport/native_viewport_impl.h"
#include "ui/gl/gl_surface.h"

using mojo::ApplicationConnection;
using mojo::Gpu;
using mojo::NativeViewport;

namespace native_viewport {

class NativeViewportAppDelegate : public mojo::ApplicationDelegate,
                                  public mojo::InterfaceFactory<NativeViewport>,
                                  public mojo::InterfaceFactory<Gpu> {
 public:
  NativeViewportAppDelegate() : is_headless_(false) {}
  ~NativeViewportAppDelegate() override {}

 private:
  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* application) override {
    app_ = application;

    mojo::TracingImpl::Create(application);

    if (app_->HasArg(mojo::kUseTestConfig))
      gfx::GLSurface::InitializeOneOffForTests();
    else if (app_->HasArg(mojo::kUseOSMesa))
      gfx::GLSurface::InitializeOneOff(gfx::kGLImplementationOSMesaGL);
    else
      gfx::GLSurface::InitializeOneOff();

    is_headless_ = app_->HasArg(mojo::kUseHeadlessConfig);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<NativeViewport>(this);
    connection->AddService<Gpu>(this);
    return true;
  }

  // mojo::InterfaceFactory<NativeViewport> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<NativeViewport> request) override {
    new NativeViewportImpl(app_, is_headless_, request.Pass());
  }

  // mojo::InterfaceFactory<Gpu> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<Gpu> request) override {
    if (!gpu_state_.get())
      gpu_state_ = new gles2::GpuImpl::State;
    new gles2::GpuImpl(request.Pass(), gpu_state_);
  }

  mojo::ApplicationImpl* app_;
  scoped_refptr<gles2::GpuImpl::State> gpu_state_;
  bool is_headless_;
  DISALLOW_COPY_AND_ASSIGN(NativeViewportAppDelegate);
};
}

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new native_viewport::NativeViewportAppDelegate);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(shell_handle);
}
