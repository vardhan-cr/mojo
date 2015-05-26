// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/gpu/texture_uploader.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
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

class KeyboardDelegate : public mojo::ApplicationDelegate,
                         public keyboard::KeyboardClient,
                         public mojo::ViewManagerDelegate,
                         public mojo::ViewObserver,
                         public mojo::TextureUploader::Client {
 public:
  KeyboardDelegate()
      : binding_(this),
        shell_(nullptr),
        keyboard_view_(nullptr),
        text_view_(nullptr),
        text_view_height_(0) {}

  ~KeyboardDelegate() override {}

  // mojo::ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override {
    shell_ = app->shell();
    view_manager_client_factory_.reset(
        new mojo::ViewManagerClientFactory(shell_, this));

    gl_context_ = mojo::GLContext::Create(shell_);
    gr_context_.reset(new mojo::GaneshContext(gl_context_));
    texture_uploader_.reset(
        new mojo::TextureUploader(this, shell_, gl_context_));
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

    root->AddChild(keyboard_view_);
    keyboard_view_->SetVisible(true);

    root->AddChild(text_view_);
    text_view_->SetVisible(true);

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
    keyboard_->Show(keyboard_client.Pass());

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
    std::string combined(text_[1]);
    combined.append(text);
    SkRect bounds;
    text_paint_.measureText(combined.data(), combined.size(), &bounds);
    if (bounds.width() > text_view_->bounds().width) {
      text_[0] = text_[1];
      text_[1] = text;
    } else {
      text_[1].append(text);
    }
    DrawText();
  }

  void DeleteSurroundingText(int32_t before_length,
                             int32_t after_length) override {
    // treat negative and zero |before_length| values as no-op.
    if (before_length > 0) {
      if (before_length > static_cast<int32_t>(text_[1].size())) {
        before_length = text_[1].size();
      }
      text_[1].erase(text_[1].end() - before_length, text_[1].end());
    }
    DrawText();
  }

  void SetComposingRegion(int32_t start, int32_t end) override { DrawText(); }

  void SetComposingText(const mojo::String& text,
                        int32_t new_cursor_position) override {
    DrawText();
  }

  void SetSelection(int32_t start, int32_t end) override { DrawText(); }

  // TextureUploader::Client implementation.
  void OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) override {
    text_view_->SetSurfaceId(surface_id.Pass());
  }

  void OnFrameComplete() override {}

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

    mojo::Rect keyboard_bounds;
    keyboard_bounds.x = root_bounds.x;
    keyboard_bounds.y = root_bounds.y + text_view_height_;
    keyboard_bounds.width = root_bounds.width;
    keyboard_bounds.height = root_bounds.height - text_view_height_;
    keyboard_view_->SetBounds(keyboard_bounds);

    mojo::Rect text_view_bounds;
    text_view_bounds.x = root_bounds.x;
    text_view_bounds.y = root_bounds.y;
    text_view_bounds.width = root_bounds.width;
    text_view_bounds.height = text_view_height_;
    text_view_->SetBounds(text_view_bounds);
  }

  void DrawText() {
    mojo::GaneshContext::Scope scope(gr_context_.get());
    mojo::GaneshSurface surface(
        gr_context_.get(), make_scoped_ptr(new mojo::GLTexture(
                               gl_context_, ToSize(text_view_->bounds()))));

    gr_context_.get()->gr()->resetContext(kTextureBinding_GrGLBackendState);
    SkCanvas* canvas = surface.canvas();

    canvas->clear(SK_ColorRED);

    float row_height = text_view_height_ / 2.0f;
    float text_baseline_offset = row_height / 5.0f;

    if (!text_[0].empty()) {
      canvas->drawText(text_[0].data(), text_[0].size(), 0.0f,
                       row_height - text_baseline_offset, text_paint_);
    }

    if (!text_[1].empty()) {
      canvas->drawText(text_[1].data(), text_[1].size(), 0.0f,
                       (2.0f * row_height) - text_baseline_offset, text_paint_);
    }

    canvas->flush();

    texture_uploader_.get()->Upload(surface.TakeTexture());
  }

 private:
  mojo::Binding<keyboard::KeyboardClient> binding_;
  keyboard::KeyboardServicePtr keyboard_;
  scoped_ptr<mojo::ViewManagerClientFactory> view_manager_client_factory_;
  mojo::Shell* shell_;
  mojo::View* keyboard_view_;
  mojo::View* text_view_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  scoped_ptr<mojo::GaneshContext> gr_context_;
  scoped_ptr<mojo::TextureUploader> texture_uploader_;
  int text_view_height_;
  std::string text_[2];
  SkPaint text_paint_;

  DISALLOW_COPY_AND_ASSIGN(KeyboardDelegate);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::KeyboardDelegate);
  return runner.Run(application_request);
}
