// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
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
#include "mojo/services/ozone_drm_gpu/public/interfaces/ozone_drm_gpu.mojom.h"
#include "mojo/services/ozone_drm_host/public/interfaces/ozone_drm_host.mojom.h"
#include "services/gles2/gpu_impl.h"
#include "services/native_viewport/native_viewport_impl.h"
#include "services/native_viewport/ozone_drm_gpu_impl.h"
#include "services/native_viewport/ozone_drm_host_impl.h"
#include "services/native_viewport/display_manager.h"
#include "ui/events/event_switches.h"
#include "ui/gl/gl_surface.h"
#include "ui/ozone/public/ozone_platform.h"

using mojo::ApplicationConnection;
using mojo::Gpu;
using mojo::NativeViewport;
using mojo::OzoneDrmGpu;
using mojo::OzoneDrmHost;

namespace native_viewport {

class NativeViewportAppDelegate : public mojo::ApplicationDelegate,
                                  public mojo::InterfaceFactory<NativeViewport>,
                                  public mojo::InterfaceFactory<Gpu> {
 public:
  NativeViewportAppDelegate() : is_headless_(false) {}
  ~NativeViewportAppDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* application) override {
    LOG(INFO) << "NativeViewportAppDelegate::Initialize";
    tracing_.Initialize(application);

    // Apply the switch for kTouchEvents to CommandLine (if set). This allows
    // redirecting the mouse to a touch device on X for testing.
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    const std::string touch_event_string("--" +
                                         std::string(switches::kTouchDevices));
    auto touch_iter = std::find(application->args().begin(),
                                application->args().end(),
                                touch_event_string);
    if (touch_iter != application->args().end() &&
        ++touch_iter != application->args().end()) {
      command_line->AppendSwitchASCII(touch_event_string, *touch_iter);
    }
    
    is_headless_ = application->HasArg(mojo::kUseHeadlessConfig);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<NativeViewport>(this);
    connection->AddService<Gpu>(this);
    return true;
  }

  // mojo::InterfaceFactory<NativeViewport> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<NativeViewport> request) override {
    LOG(INFO) << "native_viewport receiving viewport interface request";
    
    auto native_viewport = new NativeViewportImpl(is_headless_, request.Pass());
    if (gpu_state_.get()) {
      native_viewport->SetGpuState(gpu_state_);
    } else {
      native_viewport_pending_list_.push_back(native_viewport);
    }
  }
  
  // mojo::InterfaceFactory<Gpu> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<Gpu> request) override {
    LOG(INFO) << "native_viewport receiving gpu interface request";
    // PROBLEM
    init_gpu_state();
    new gles2::GpuImpl(request.Pass(), gpu_state_);
  }

  void init_gpu_state() {
    if (gpu_state_.get())
      return;

//    if (application->HasArg(mojo::kUseTestConfig))
//      gfx::GLSurface::InitializeOneOffForTests();
//    else if (application->HasArg(mojo::kUseOSMesa))
//      gfx::GLSurface::InitializeOneOff(gfx::kGLImplementationOSMesaGL);
//    else
      gfx::GLSurface::InitializeOneOff();
    
    gpu_state_ = new gles2::GpuState;

    for (auto native_viewport : native_viewport_pending_list_) {
      native_viewport->SetGpuState(gpu_state_);
    }
  }

  scoped_refptr<gles2::GpuState> gpu_state_;
  bool is_headless_;
  mojo::TracingImpl tracing_;
  std::vector<NativeViewportImpl*> native_viewport_pending_list_;
  
  DISALLOW_COPY_AND_ASSIGN(NativeViewportAppDelegate);
};

class NativeViewportOzoneDrmAppDelegate : public NativeViewportAppDelegate,
                                          public mojo::InterfaceFactory<OzoneDrmHost>,
                                          public mojo::InterfaceFactory<OzoneDrmGpu> {  
public:
  using NativeViewportAppDelegate::Create;

  void Initialize(mojo::ApplicationImpl* application) override {
    NativeViewportAppDelegate::Initialize(application);
    application->ConnectToService("mojo:native_viewport_service", &ozone_drm_gpu_);    
    application->ConnectToService("mojo:native_viewport_service", &ozone_drm_host_);
  }

  void DisplayManagerQuit() {
    LOG(INFO) << "DisplayManagerQuit";
  }
  
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    if (!NativeViewportAppDelegate::ConfigureIncomingConnection(connection))
      return false;
    connection->AddService<OzoneDrmGpu>(this);
    connection->AddService<OzoneDrmHost>(this);    
    return true;
  }
  
  // mojo::InterfaceFactory<OzoneDrmGpu> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<OzoneDrmGpu> request) override {
    LOG(INFO) << "native_viewport receiving OzoneDrmGpu interface request";    
    new OzoneDrmGpuImpl(request.Pass(),
                        ozone_drm_host_,
                        base::Bind(&NativeViewportOzoneDrmAppDelegate::OnGraphicsDeviceAdded, base::Unretained(this)));
  }

  // mojo::InterfaceFactory<OzoneDrmHost> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<OzoneDrmHost> request) override {
    LOG(INFO) << "native_viewport receiving OzoneDrmHost interface request";
    ui::OzonePlatform::InitializeForUI();
    new OzoneDrmHostImpl(request.Pass(), ozone_drm_gpu_);

    display_manager_.reset(
      new DisplayManager(base::Bind(&NativeViewportOzoneDrmAppDelegate::DisplayManagerQuit,
                                    base::Unretained(this))));    
  }
  
  void OnGraphicsDeviceAdded() {
    LOG(INFO) << "native_viewport graphics device added";
    init_gpu_state();
  }

  std::unique_ptr<DisplayManager> display_manager_;
  mojo::OzoneDrmHostPtr ozone_drm_host_;
  mojo::OzoneDrmGpuPtr ozone_drm_gpu_;
};

} // namespace


MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new native_viewport::NativeViewportOzoneDrmAppDelegate);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(application_request);
}
