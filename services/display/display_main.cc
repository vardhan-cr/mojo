// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

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
#include "mojo/services/ozone_drm_host/public/interfaces/ozone_drm_host.mojom.h"
#include "mojo/services/ozone_drm_gpu/public/interfaces/ozone_drm_gpu.mojom.h"
#include "services/display/ozone_drm_host_impl.h"
#include "ui/ozone/public/ozone_platform.h"

using mojo::ApplicationConnection;
using mojo::OzoneDrmHost;

namespace display {

class DisplayAppDelegate : public mojo::ApplicationDelegate,
                           public mojo::InterfaceFactory<OzoneDrmHost> {
 public:
  DisplayAppDelegate() {}
  ~DisplayAppDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    LOG(INFO) << "DisplayAppDelegate::Initialize";
    tracing_.Initialize(app);

    // Hmm this is done in the platform window
    ui::OzonePlatform::InitializeForUI();
    
    app->ConnectToService("mojo:native_viewport_service", &ozone_drm_gpu_);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<OzoneDrmHost>(this);
    return true;
  }

  // mojo::InterfaceFactory<OzoneDrmHost> implementation.
  void Create(ApplicationConnection* connection,
              mojo::InterfaceRequest<OzoneDrmHost> request) override {
    new OzoneDrmHostImpl(request.Pass(), ozone_drm_gpu_);
  }

 private:
  mojo::TracingImpl tracing_;
  mojo::OzoneDrmGpuPtr ozone_drm_gpu_;
  
  DISALLOW_COPY_AND_ASSIGN(DisplayAppDelegate);
};

} // namespace


MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new display::DisplayAppDelegate);
  runner.set_message_loop_type(base::MessageLoop::TYPE_UI);
  return runner.Run(application_request);
}
