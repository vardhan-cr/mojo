// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>

#include "base/bind.h"
#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/gpu/gl_texture.h"
#include "mojo/gpu/texture_cache.h"
#include "mojo/gpu/texture_uploader.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"
#include "mojo/skia/ganesh_surface.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace examples {

mojo::Size ToSize(const mojo::Rect& rect) {
  mojo::Size size;
  size.width = rect.width;
  size.height = rect.height;
  return size;
}

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
        draw_callback_(draw_callback),
        weak_factory_(this) {
    gr_context_.reset(new mojo::GaneshContext(gl_context_));
    submit_frame_callback_ = base::Bind(&ViewTextureUploader::OnFrameComplete,
                                        weak_factory_.GetWeakPtr());
  }

  void Draw(uint32_t surface_id) {
    mojo::GaneshContext::Scope scope(gr_context_.get());

    mojo::Size view_size = ToSize(view_->bounds());
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
  base::WeakPtrFactory<ViewTextureUploader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewTextureUploader);
};

class KeyboardDelegate : public mojo::ApplicationDelegate,
                         public keyboard::KeyboardClient,
                         public mojo::ViewManagerDelegate,
                         public mojo::ViewObserver {
 public:
  KeyboardDelegate()
      : binding_(this),
        shell_(nullptr),
        keyboard_view_(nullptr),
        text_view_(nullptr),
        root_view_(nullptr),
        root_view_surface_id_(1u),
        text_view_surface_id_(2u),
        id_namespace_(0u),
        text_view_height_(0),
        weak_factory_(this) {}

  ~KeyboardDelegate() override {}

  void SetIdNamespace(uint32_t id_namespace) {
    id_namespace_ = id_namespace;
    UpdateSurfaceIds();
  }

  void UpdateSurfaceIds() {
    auto text_view_surface_id = mojo::SurfaceId::New();
    text_view_surface_id->id_namespace = id_namespace_;
    text_view_surface_id->local = text_view_surface_id_;
    text_view_->SetSurfaceId(text_view_surface_id.Pass());

    auto root_view_surface_id = mojo::SurfaceId::New();
    root_view_surface_id->id_namespace = id_namespace_;
    root_view_surface_id->local = root_view_surface_id_;
    root_view_->SetSurfaceId(root_view_surface_id.Pass());
  }

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(shell_, this));
  }

  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // mojo::ViewManagerDelegate implementation.
  void OnEmbed(mojo::View* root,
               mojo::InterfaceRequest<mojo::ServiceProvider> services,
               mojo::ServiceProviderPtr exposed_services) override {
    root_view_ = root;
    root->AddObserver(this);

    keyboard_view_ = root->view_manager()->CreateView();

    text_view_ = root->view_manager()->CreateView();

    skia::RefPtr<SkTypeface> typeface = skia::AdoptRef(
        SkTypeface::CreateFromName("Arial", SkTypeface::kNormal));
    text_paint_.setTypeface(typeface.get());
    text_paint_.setColor(SK_ColorBLACK);
    text_paint_.setAntiAlias(true);
    text_paint_.setTextAlign(SkPaint::kLeft_Align);

    UpdateViewBounds(root->bounds());

    root->AddChild(text_view_);
    text_view_->SetVisible(true);

    root->AddChild(keyboard_view_);
    keyboard_view_->SetVisible(true);

    mojo::ServiceProviderPtr keyboard_sp;
    keyboard_view_->Embed("mojo:keyboard_native", GetProxy(&keyboard_sp),
                          nullptr);
    keyboard_sp->ConnectToService(keyboard::KeyboardService::Name_,
                                  GetProxy(&keyboard_).PassMessagePipe());

    keyboard_->ShowByRequest();
    keyboard_->Hide();

    keyboard::KeyboardClientPtr keyboard_client;
    auto request = mojo::GetProxy(&keyboard_client);
    binding_.Bind(request.Pass());
    keyboard_->Show(keyboard_client.Pass(), keyboard::KeyboardType::TEXT);

    mojo::ServiceProviderPtr surfaces_service_provider;
    shell_->ConnectToApplication("mojo:surfaces_service",
                                 mojo::GetProxy(&surfaces_service_provider),
                                 nullptr);
    mojo::ConnectToService(surfaces_service_provider.get(), &surface_);
    gl_context_ = mojo::GLContext::Create(shell_);

    surface_->CreateSurface(root_view_surface_id_);
    surface_->CreateSurface(text_view_surface_id_);
    surface_->GetIdNamespace(
        base::Bind(&KeyboardDelegate::SetIdNamespace, base::Unretained(this)));

    mojo::ResourceReturnerPtr resource_returner;
    texture_cache_.reset(
        new mojo::TextureCache(gl_context_, &resource_returner));
    text_view_texture_uploader_.reset(new ViewTextureUploader(
        gl_context_, &surface_, text_view_, texture_cache_.get(),
        base::Bind(&KeyboardDelegate::DrawTextView,
                   weak_factory_.GetWeakPtr())));
    root_view_texture_uploader_.reset(new ViewTextureUploader(
        gl_context_, &surface_, root, texture_cache_.get(),
        base::Bind(&KeyboardDelegate::DrawRootView,
                   weak_factory_.GetWeakPtr())));
    surface_->SetResourceReturner(resource_returner.Pass());

    DrawText();
  }

  void OnViewManagerDisconnected(mojo::ViewManager* view_manager) override {
  }

  // keyboard::KeyboardClient implementation.
  void CommitCompletion(keyboard::CompletionDataPtr completion) override {
    DrawText();
  }

  void CommitCorrection(keyboard::CorrectionDataPtr correction) override {
    DrawText();
  }

  void CommitText(const mojo::String& text,
                  int32_t new_cursor_position) override {
    text_.append(text);
    DrawText();
  }

  void DeleteSurroundingText(int32_t before_length,
                             int32_t after_length) override {
    // treat negative and zero |before_length| values as no-op.
    if (before_length > 0) {
      if (before_length > static_cast<int32_t>(text_.size())) {
        before_length = text_.size();
      }
      text_.erase(text_.end() - before_length, text_.end());
    }
    DrawText();
  }

  void SetComposingRegion(int32_t start, int32_t end) override { DrawText(); }

  void SetComposingText(const mojo::String& text,
                        int32_t new_cursor_position) override {
    DrawText();
  }

  void SetSelection(int32_t start, int32_t end) override { DrawText(); }

  // mojo::ViewObserver implementation.
  void OnViewDestroyed(mojo::View* view) override {
    if (view == text_view_) {
      text_view_->RemoveObserver(this);
      text_view_ = nullptr;
    }
  }

  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override {
    surface_->DestroySurface(root_view_surface_id_);
    surface_->DestroySurface(text_view_surface_id_);
    root_view_surface_id_ += 2;
    text_view_surface_id_ += 2;
    surface_->CreateSurface(root_view_surface_id_);
    surface_->CreateSurface(text_view_surface_id_);
    UpdateSurfaceIds();
    UpdateViewBounds(new_bounds);
    DrawText();
  }

  void UpdateViewBounds(const mojo::Rect& root_bounds) {
    float keyboard_height = (root_bounds.width > root_bounds.height)
                                ? root_bounds.width * 0.3f
                                : root_bounds.width * 0.6f;
    text_view_height_ = root_bounds.height - keyboard_height;
    float row_height = text_view_height_ / 2.0f;
    text_paint_.setTextSize(row_height / 1.7f);

    // One third of the keyboard view's height will be used for overlapping
    // views behind it (text_view_ in this case).  Thus create the keyboard 50%
    // taller than the area we want the keys to be drawn to.
    float overlap = keyboard_height / 2;
    mojo::Rect keyboard_bounds;
    keyboard_bounds.x = root_bounds.x;
    keyboard_bounds.y = root_bounds.y + text_view_height_ - overlap;
    keyboard_bounds.width = root_bounds.width;
    keyboard_bounds.height = root_bounds.height - text_view_height_ + overlap;
    keyboard_view_->SetBounds(keyboard_bounds);

    mojo::Rect text_view_bounds;
    text_view_bounds.x = root_bounds.x;
    text_view_bounds.y = root_bounds.y;
    text_view_bounds.width = root_bounds.width;
    text_view_bounds.height = text_view_height_;
    text_view_->SetBounds(text_view_bounds);
  }

  void DrawText() {
    if (root_view_texture_uploader_) {
      root_view_texture_uploader_->Draw(root_view_surface_id_);
    }
    if (text_view_texture_uploader_) {
      text_view_texture_uploader_->Draw(text_view_surface_id_);
    }
  }

  void DrawTextView(SkCanvas* canvas) {
    canvas->clear(SK_ColorRED);

    float row_height = text_view_height_ / 2.0f;
    float text_baseline_offset = row_height / 5.0f;
    if (!text_.empty()) {
      SkRect sk_rect;
      text_paint_.measureText((const void*)(text_.c_str()), text_.length(),
                              &sk_rect);

      if (sk_rect.width() > text_view_->bounds().width) {
        std::string reverse_text = text_;
        std::reverse(reverse_text.begin(), reverse_text.end());

        size_t processed1 = text_paint_.breakText(
            (const void*)(reverse_text.c_str()), strlen(reverse_text.c_str()),
            text_view_->bounds().width);
        size_t processed2 = text_paint_.breakText(
            (const void*)(reverse_text.c_str() + processed1),
            strlen(reverse_text.c_str()) - processed1,
            text_view_->bounds().width);
        if (processed1 + processed2 < text_.length()) {
          DrawSecondLine(canvas, text_.length() - processed1, processed1,
                         row_height, text_baseline_offset);
          DrawFirstLine(canvas, text_.length() - processed1 - processed2,
                        processed2, row_height, text_baseline_offset);
        } else {
          size_t processed3 =
              text_paint_.breakText((const void*)(text_.c_str()),
                                    text_.length(), text_view_->bounds().width);
          DrawFirstLine(canvas, 0, processed3, row_height,
                        text_baseline_offset);
          DrawSecondLine(canvas, processed3, text_.length() - processed3,
                         row_height, text_baseline_offset);
        }
      } else {
        DrawSecondLine(canvas, 0, text_.length(), row_height,
                       text_baseline_offset);
      }
    }

    canvas->flush();
  }

  void DrawFirstLine(SkCanvas* canvas,
                     const size_t offset,
                     const size_t length,
                     const float row_height,
                     const float text_baseline_offset) {
    canvas->drawText(text_.data() + offset, length, 0.0f,
                     row_height - text_baseline_offset, text_paint_);
  }

  void DrawSecondLine(SkCanvas* canvas,
                      const size_t offset,
                      const size_t length,
                      const float row_height,
                      const float text_baseline_offset) {
    canvas->drawText(text_.data() + offset, length, 0.0f,
                     (2.0f * row_height) - text_baseline_offset, text_paint_);
  }

  void DrawRootView(SkCanvas* canvas) {
    canvas->clear(SK_ColorDKGRAY);
    canvas->flush();
  }

 private:
  mojo::Binding<keyboard::KeyboardClient> binding_;
  keyboard::KeyboardServicePtr keyboard_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;
  mojo::Shell* shell_;
  mojo::View* keyboard_view_;
  mojo::View* text_view_;
  mojo::View* root_view_;
  uint32_t root_view_surface_id_;
  uint32_t text_view_surface_id_;
  uint32_t id_namespace_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  mojo::SurfacePtr surface_;
  scoped_ptr<mojo::TextureCache> texture_cache_;
  scoped_ptr<ViewTextureUploader> text_view_texture_uploader_;
  scoped_ptr<ViewTextureUploader> root_view_texture_uploader_;
  int text_view_height_;
  std::string text_;
  SkPaint text_paint_;
  base::WeakPtrFactory<KeyboardDelegate> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::KeyboardDelegate);
  return runner.Run(application_request);
}
