// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/message_loop/message_loop.h"
#include "cc/surfaces/surface_id_allocator.h"
#include "examples/surfaces_app/child.mojom.h"
#include "examples/surfaces_app/embedder.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/gpu/public/interfaces/command_buffer.mojom.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/display.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "ui/gfx/rect.h"

namespace mojo {
namespace examples {

class SurfacesApp : public ApplicationDelegate {
 public:
  SurfacesApp() : app_impl_(nullptr), id_namespace_(0u), weak_factory_(this) {}
  ~SurfacesApp() override {}

  // ApplicationDelegate implementation
  void Initialize(ApplicationImpl* app) override {
    app_impl_ = app;
    size_ = gfx::Size(800, 600);

    // Connect to the native viewport service and create a viewport.
    app_impl_->ConnectToService("mojo:native_viewport_service", &viewport_);
    viewport_->Create(Size::From(size_), SurfaceConfiguration::New(),
                      [](ViewportMetricsPtr metrics) {});
    viewport_->Show();

    // Grab a ContextProvider associated with the viewport.
    ContextProviderPtr onscreen_context_provider;
    viewport_->GetContextProvider(GetProxy(&onscreen_context_provider));

    // Create a surfaces Display bound to the viewport's context provider.
    DisplayFactoryPtr display_factory;
    app_impl_->ConnectToService("mojo:surfaces_service", &display_factory);
    display_factory->Create(onscreen_context_provider.Pass(),
                            nullptr,  // resource_returner
                            GetProxy(&display_));

    // Construct a mojo::examples::Embedder object that will draw to our
    // display.
    embedder_.reset(new Embedder(display_.get()));

    child_size_ = gfx::Size(size_.width() / 3, size_.height() / 2);
    app_impl_->ConnectToService("mojo:surfaces_child_app", &child_one_);
    app_impl_->ConnectToService("mojo:surfaces_child_gl_app", &child_two_);
    child_one_->ProduceFrame(Color::From(SK_ColorBLUE),
                             Size::From(child_size_),
                             base::Bind(&SurfacesApp::ChildOneProducedFrame,
                                        base::Unretained(this)));
    child_two_->ProduceFrame(Color::From(SK_ColorGREEN),
                             Size::From(child_size_),
                             base::Bind(&SurfacesApp::ChildTwoProducedFrame,
                                        base::Unretained(this)));
    Draw(10);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    return false;
  }

  void ChildOneProducedFrame(SurfaceIdPtr id) {
    child_one_id_ = id.To<cc::SurfaceId>();
  }

  void ChildTwoProducedFrame(SurfaceIdPtr id) {
    child_two_id_ = id.To<cc::SurfaceId>();
  }

  void Draw(int offset) {
    int bounced_offset = offset;
    if (offset > 200)
      bounced_offset = 400 - offset;
    if (!embedder_->frame_pending()) {
      embedder_->ProduceFrame(child_one_id_, child_two_id_, child_size_, size_,
                              bounced_offset);
    }
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(
            &SurfacesApp::Draw, base::Unretained(this), (offset + 2) % 400),
        base::TimeDelta::FromMilliseconds(50));
  }

 private:

  ApplicationImpl* app_impl_;
  DisplayPtr display_;
  uint32_t id_namespace_;
  scoped_ptr<Embedder> embedder_;
  ChildPtr child_one_;
  cc::SurfaceId child_one_id_;
  ChildPtr child_two_;
  cc::SurfaceId child_two_id_;
  gfx::Size size_;
  gfx::Size child_size_;

  NativeViewportPtr viewport_;

  base::WeakPtrFactory<SurfacesApp> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(SurfacesApp);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::SurfacesApp);
  return runner.Run(application_request);
}
