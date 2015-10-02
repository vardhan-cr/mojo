// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/binding_set.h"
#include "mojo/gpu/gl_texture.h"
#include "mojo/gpu/texture_cache.h"
#include "mojo/gpu/texture_uploader.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/nfc/public/interfaces/nfc.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"
#include "mojo/skia/ganesh_surface.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/size.h"

namespace examples {

mojo::Size ToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

struct CircleData {
  gfx::Point point;
  SkColor color;
};

class ViewTextureUploader {
 public:
  ViewTextureUploader(base::WeakPtr<mojo::GLContext> gl_context,
                      mojo::SurfacePtr* surface,
                      mojo::View* view,
                      mojo::TextureCache* texture_cache,
                      base::Callback<void(SkCanvas*)> draw_callback)
      : gl_context_(gl_context),
        surface_(surface),
        texture_cache_(texture_cache),
        view_(view),
        draw_callback_(draw_callback) {
    gr_context_.reset(new mojo::GaneshContext(gl_context_));
    submit_frame_callback_ = base::Bind(&ViewTextureUploader::OnFrameComplete,
                                        base::Unretained(this));
  }

  void Draw(uint32_t surface_id) {
    mojo::GaneshContext::Scope scope(gr_context_.get());

    mojo::Size view_size = ToSize(view_->bounds());
    if (view_size.width <= 0 || view_size.height <= 0) {
      return;
    }
    scoped_ptr<mojo::TextureCache::TextureInfo> texture_info(
        texture_cache_->GetTexture(view_size).Pass());
    mojo::GaneshSurface ganesh_surface(gr_context_.get(),
                                       texture_info->Texture().Pass());

    gr_context_->gr()->resetContext(kTextureBinding_GrGLBackendState);
    SkCanvas* canvas = ganesh_surface.canvas();
    draw_callback_.Run(canvas);

    scoped_ptr<mojo::GLTexture> surface_texture(
        ganesh_surface.TakeTexture().Pass());
    mojo::FramePtr frame = mojo::TextureUploader::GetUploadFrame(
        gl_context_, texture_info->ResourceId(), surface_texture);
    texture_cache_->NotifyPendingResourceReturn(texture_info->ResourceId(),
                                                surface_texture.Pass());
    (*surface_)->SubmitFrame(surface_id, frame.Pass(), submit_frame_callback_);
  }

  void OnFrameComplete() {}

 private:
  base::WeakPtr<mojo::GLContext> gl_context_;
  scoped_ptr<mojo::GaneshContext> gr_context_;
  mojo::SurfacePtr* surface_;
  mojo::TextureCache* texture_cache_;
  mojo::View* view_;
  base::Callback<void(SkCanvas*)> draw_callback_;
  base::Closure submit_frame_callback_;

  DISALLOW_COPY_AND_ASSIGN(ViewTextureUploader);
};

class NfcSenderDelegate : public mojo::ApplicationDelegate,
                          public mojo::ViewManagerDelegate,
                          public nfc::NfcReceiver,
                          public mojo::ViewObserver,
                          public mojo::InterfaceFactory<nfc::NfcReceiver> {
 public:
  NfcSenderDelegate()
      : app_(nullptr),
        shell_(nullptr),
        view_(nullptr),
        nfc_receiver_binding_(this),
        surface_id_(1u),
        transmission_pending_(false) {}

  ~NfcSenderDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    app_ = app;
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(app->shell(), this));

    ConnectToNfc();
    TransmitOnNextConnection();
  }

  void ConnectToNfc() {
    app_->ConnectToService("mojo:nfc", &nfc_);
    nfc_.set_connection_error_handler(base::Bind(
        &NfcSenderDelegate::OnConnectionError, base::Unretained(this)));

    nfc_->Register();
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    connection->AddService(this);
    return true;
  }

  // mojo::InterfaceFactory<nfc::NfcReceiver> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<nfc::NfcReceiver> request) override {
    nfc_receiver_bindings_.AddBinding(this, request.Pass());
  }

  // mojo::ViewManagerDelegate
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    view_ = root;
    view_->AddObserver(this);

    mojo::ServiceProviderPtr surfaces_service_provider;
    shell_->ConnectToApplication("mojo:surfaces_service",
                                 mojo::GetProxy(&surfaces_service_provider),
                                 nullptr);
    mojo::ConnectToService(surfaces_service_provider.get(), &surface_);
    gl_context_ = mojo::GLContext::Create(shell_);

    surface_->CreateSurface(surface_id_);
    surface_->GetIdNamespace(
        base::Bind(&NfcSenderDelegate::SetIdNamespace, base::Unretained(this)));

    mojo::ResourceReturnerPtr resource_returner;
    texture_cache_.reset(
        new mojo::TextureCache(gl_context_, &resource_returner));
    texture_uploader_.reset(new ViewTextureUploader(
        gl_context_, &surface_, view_, texture_cache_.get(),
        base::Bind(&NfcSenderDelegate::DrawCircles, base::Unretained(this))));
    surface_->SetResourceReturner(resource_returner.Pass());
    UpdateView();
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {}

  void OnConnectionError() {
    transmission_pending_ = false;
    ConnectToNfc();
  }

  void SetIdNamespace(uint32_t id_namespace) {
    auto view_surface_id = mojo::SurfaceId::New();
    view_surface_id->id_namespace = id_namespace;
    view_surface_id->local = surface_id_;
    view_->SetSurfaceId(view_surface_id.Pass());
  }

  void UpdateView() {
    if (texture_uploader_) {
      texture_uploader_->Draw(surface_id_);
    }
  }

  void DrawCircles(SkCanvas* canvas) {
    canvas->clear(SK_ColorGREEN);
    for (const auto& circle : circles_) {
      SkPaint paint;
      paint.setColor(circle.color);
      paint.setFlags(SkPaint::kAntiAlias_Flag);
      canvas->drawCircle(circle.point.x(), circle.point.y(), 50, paint);
    }

    canvas->flush();
  }

  // mojo::ViewObserver
  void OnViewInputEvent(mojo::View* view,
                        const mojo::EventPtr& event) override {
    if (event->action == mojo::EventType::POINTER_DOWN ||
        event->action == mojo::EventType::POINTER_MOVE) {
      if (event->pointer_data) {
        gfx::Point point(event->pointer_data->x, event->pointer_data->y);

        int r = rand() % 256;
        int g = rand() % 256;
        int b = rand() % 256;
        CircleData circle_data;
        circle_data.point = point;
        circle_data.color = SkColorSetARGB(0xFF, r, g, b);
        circles_.push_back(circle_data);
        while (circles_.size() > 100) {
          circles_.pop_front();
        }
        UpdateView();
        TransmitOnNextConnection();
      }
    }
  }

  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override {
    UpdateView();
  }

  // nfc::NfcReceiver implementation.
  void OnReceivedNfcData(nfc::NfcDataPtr nfc_data) override {
    if (nfc_data->data.is_null()) {
      return;
    }
    circles_.clear();

    for (size_t offset = 0; (offset + 12) <= nfc_data->data.size();
         offset += 12) {
      int32_t x;
      memcpy(&x, &nfc_data->data.front() + offset, 4);

      int32_t y;
      memcpy(&y, &nfc_data->data.front() + offset + 4, 4);

      CircleData circle_data;
      circle_data.point = gfx::Point(x, y);
      circle_data.color = SkColorSetARGB(
          nfc_data->data.at(offset + 8), nfc_data->data.at(offset + 9),
          nfc_data->data.at(offset + 10), nfc_data->data.at(offset + 11));

      circles_.push_back(circle_data);
    }
    UpdateView();
    TransmitOnNextConnection();
  }

  void TransmitOnNextConnection() {
    if (transmission_pending_) {
      nfc_transmission_->Cancel();
    }
    transmission_pending_ = true;
    nfc_->TransmitOnNextConnection(
        CreateNfcData().Pass(), GetProxy(&nfc_transmission_),
        base::Bind(&NfcSenderDelegate::TransmitResult, base::Unretained(this)));
  }

  void TransmitResult(bool success) {
    transmission_pending_ = false;
    if (success) {
      TransmitOnNextConnection();
    }
  }

  nfc::NfcDataPtr CreateNfcData() {
    nfc::NfcDataPtr nfc_data = nfc::NfcData::New();
    nfc_data->data = mojo::Array<uint8>::New(12 * circles_.size());
    size_t offset = 0;
    for (const auto& circle : circles_) {
      int32_t x = circle.point.x();
      memcpy(&nfc_data->data.front() + offset, &x, 4);

      int32_t y = circle.point.y();
      memcpy(&nfc_data->data.front() + offset + 4, &y, 4);

      nfc_data->data.at(offset + 8) = SkColorGetA(circle.color);
      nfc_data->data.at(offset + 9) = SkColorGetR(circle.color);
      nfc_data->data.at(offset + 10) = SkColorGetG(circle.color);
      nfc_data->data.at(offset + 11) = SkColorGetB(circle.color);
      offset += 12;
    }
    return nfc_data;
  }

 private:
  mojo::ApplicationImpl* app_;
  mojo::Shell* shell_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;
  mojo::View* view_;
  nfc::NfcPtr nfc_;
  nfc::NfcTransmissionPtr nfc_transmission_;
  mojo::Binding<nfc::NfcReceiver> nfc_receiver_binding_;
  std::deque<CircleData> circles_;
  uint32_t surface_id_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  mojo::SurfacePtr surface_;
  scoped_ptr<mojo::TextureCache> texture_cache_;
  scoped_ptr<ViewTextureUploader> texture_uploader_;
  bool transmission_pending_;
  mojo::BindingSet<nfc::NfcReceiver> nfc_receiver_bindings_;

  DISALLOW_COPY_AND_ASSIGN(NfcSenderDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::NfcSenderDelegate);
  return runner.Run(application_request);
}
