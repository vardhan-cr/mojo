// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "examples/bitmap_uploader/bitmap_uploader.h"
#include "examples/wm_flow/app/embedder.mojom.h"
#include "examples/wm_flow/embedded/embeddee.mojom.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "third_party/skia/include/core/SkColor.h"

namespace examples {

namespace {

class EmbeddeeImpl : public Embeddee {
 public:
  explicit EmbeddeeImpl(mojo::InterfaceRequest<Embeddee> request)
      : binding_(this, request.Pass()) {}
  ~EmbeddeeImpl() override {}

 private:
  // Overridden from Embeddee:
  void HelloBack(const mojo::Callback<void()>& callback) override {
    callback.Run();
  }

  mojo::StrongBinding<Embeddee> binding_;
  DISALLOW_COPY_AND_ASSIGN(EmbeddeeImpl);
};

}  // namespace

class WMFlowEmbedded : public mojo::ApplicationDelegate,
                       public mojo::ViewManagerDelegate,
                       public mojo::InterfaceFactory<Embeddee> {
 public:
  WMFlowEmbedded() : shell_(nullptr) {
    embeddee_provider_impl_.AddService(this);
  }
  ~WMFlowEmbedded() override {}

 private:
  // Overridden from Application:
  void Initialize(mojo::ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(app->shell(), this));
  }
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // Overridden from mojo::ViewManagerDelegate:
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    bitmap_uploader_.reset(new mojo::BitmapUploader(root));
    bitmap_uploader_->Init(shell_);
    // BitmapUploader does not track view size changes, we would
    // need to subscribe to OnViewBoundsChanged and tell the bitmap uploader
    // to invalidate itself.  This is better done once if had a per-view
    // object instead of holding per-view state on the ApplicationDelegate.
    bitmap_uploader_->SetColor(SK_ColorMAGENTA);

    embeddee_provider_impl_.Bind(services.Pass());
    // FIXME: embedder_ is wrong for the same reason the embedee_ storage is
    // wrong in app.cc.  We need separate per-instance storage not on the
    // application delegate.
    mojo::ConnectToService(exposed_services.get(), &embedder_);
    embedder_->HelloWorld(base::Bind(&WMFlowEmbedded::HelloWorldAck,
                                     base::Unretained(this)));
  }
  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {}

  // Overridden from mojo::InterfaceFactory<Embeddee>:
  void Create(mojo::ApplicationConnection* app,
              mojo::InterfaceRequest<Embeddee> request) override {
    new EmbeddeeImpl(request.Pass());
  }

  void HelloWorldAck() {
    printf("HelloWorld() ack'ed\n");
  }

  mojo::Shell* shell_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;
  EmbedderPtr embedder_;
  mojo::ServiceProviderImpl embeddee_provider_impl_;
  scoped_ptr<mojo::BitmapUploader> bitmap_uploader_;

  DISALLOW_COPY_AND_ASSIGN(WMFlowEmbedded);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new examples::WMFlowEmbedded);
  return runner.Run(shell_handle);
}

