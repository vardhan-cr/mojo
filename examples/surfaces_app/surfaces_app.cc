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
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/gpu/public/interfaces/command_buffer.mojom.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "ui/gfx/rect.h"

namespace mojo {
namespace examples {

class SurfacesApp : public ApplicationDelegate,
                    public SurfaceClient,
                    public NativeViewportClient {
 public:
  SurfacesApp() : onscreen_id_(SurfaceId::New()), weak_factory_(this) {}
  ~SurfacesApp() override {}

  // ApplicationDelegate implementation
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->ConnectToService("mojo:native_viewport_service", &viewport_);
    viewport_.set_client(this);

    connection->ConnectToService("mojo:surfaces_service", &surface_);
    surface_.set_client(this);
    embedder_.set_surface(surface_.get());

    size_ = gfx::Size(800, 600);

    viewport_->Create(Size::From(size_),
                      base::Bind(&SurfacesApp::OnCreatedNativeViewport,
                                 weak_factory_.GetWeakPtr()));
    viewport_->Show();

    child_size_ = gfx::Size(size_.width() / 3, size_.height() / 2);
    connection->ConnectToService("mojo:surfaces_child_app", &child_one_);
    connection->ConnectToService("mojo:surfaces_child_gl_app", &child_two_);
    child_one_->ProduceFrame(Color::From(SK_ColorBLUE),
                             Size::From(child_size_),
                             base::Bind(&SurfacesApp::ChildOneProducedFrame,
                                        base::Unretained(this)));
    child_two_->ProduceFrame(Color::From(SK_ColorGREEN),
                             Size::From(child_size_),
                             base::Bind(&SurfacesApp::ChildTwoProducedFrame,
                                        base::Unretained(this)));
    return true;
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
    embedder_.ProduceFrame(child_one_id_, child_two_id_, child_size_, size_,
                           bounced_offset);
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(
            &SurfacesApp::Draw, base::Unretained(this), (offset + 2) % 400),
        base::TimeDelta::FromMilliseconds(50));
  }

  // SurfaceClient implementation.
  void SetIdNamespace(uint32_t id_namespace) override {
    onscreen_id_->id_namespace = id_namespace;
    embedder_.set_surface_id(onscreen_id_.Clone());
    surface_->CreateSurface(onscreen_id_.Clone());
    viewport_->SubmittedFrame(onscreen_id_.Clone());
    Draw(10);
  }
  void ReturnResources(Array<ReturnedResourcePtr> resources) override {
    DCHECK(!resources.size());
  }
  // NativeViewportClient implementation.
  void OnMetricsChanged(mojo::ViewportMetricsPtr metrics) override {}
  void OnDestroyed() override {}

 private:
  void OnCreatedNativeViewport(uint64_t native_viewport_id) {}

  SurfacePtr surface_;
  SurfaceIdPtr onscreen_id_;
  Embedder embedder_;
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

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::SurfacesApp);
  return runner.Run(shell_handle);
}
