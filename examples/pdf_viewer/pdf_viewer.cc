// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "examples/bitmap_uploader/bitmap_uploader.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/input_events/public/interfaces/input_key_codes.mojom.h"
#include "mojo/services/view_manager/public/cpp/types.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_client_factory.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "third_party/pdfium/fpdfsdk/include/fpdf_ext.h"
#include "third_party/pdfium/fpdfsdk/include/fpdfview.h"
#include "v8/include/v8.h"

#define BACKGROUND_COLOR 0xFF888888

namespace mojo {
namespace examples {

namespace {

class EmbedderData {
 public:
  EmbedderData(Shell* shell, View* root) : bitmap_uploader_(root) {
    bitmap_uploader_.Init(shell);
    bitmap_uploader_.SetColor(BACKGROUND_COLOR);
  }

  BitmapUploader& bitmap_uploader() { return bitmap_uploader_; }

 private:
  BitmapUploader bitmap_uploader_;

  DISALLOW_COPY_AND_ASSIGN(EmbedderData);
};

}  // namespace

class PDFView : public ApplicationDelegate,
                public ViewManagerDelegate,
                public ViewObserver {
 public:
  PDFView(URLResponsePtr response)
      : current_page_(0), page_count_(0), doc_(NULL), app_(nullptr) {
    FetchPDF(response.Pass());
  }

  ~PDFView() override {
    if (doc_)
      FPDF_CloseDocument(doc_);
    for (auto& roots : embedder_for_roots_) {
      roots.first->RemoveObserver(this);
      delete roots.second;
    }
  }

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override {
    app_ = app;
    view_manager_client_factory_.reset(
        new ViewManagerClientFactory(app->shell(), this));
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(view_manager_client_factory_.get());
    return true;
  }

  // Overridden from ViewManagerDelegate:
  void OnEmbed(View* root,
               InterfaceRequest<ServiceProvider> services,
               ServiceProviderPtr exposed_services) override {
    DCHECK(embedder_for_roots_.find(root) == embedder_for_roots_.end());
    root->AddObserver(this);
    EmbedderData* embedder_data = new EmbedderData(app_->shell(), root);
    embedder_for_roots_[root] = embedder_data;
    DrawBitmap(embedder_data);
  }

  void OnViewManagerDisconnected(ViewManager* view_manager) override {}

  // Overridden from ViewObserver:
  void OnViewBoundsChanged(View* view,
                           const Rect& old_bounds,
                           const Rect& new_bounds) override {
    DCHECK(embedder_for_roots_.find(view) != embedder_for_roots_.end());
    DrawBitmap(embedder_for_roots_[view]);
  }

  void OnViewInputEvent(View* view, const EventPtr& event) override {
    DCHECK(embedder_for_roots_.find(view) != embedder_for_roots_.end());
    if (event->key_data &&
        (event->action != EVENT_TYPE_KEY_PRESSED || event->key_data->is_char)) {
      return;
    }
    if ((event->key_data &&
         event->key_data->windows_key_code == KEYBOARD_CODE_DOWN) ||
        (event->pointer_data && event->pointer_data->vertical_wheel < 0)) {
      if (current_page_ < (page_count_ - 1)) {
        current_page_++;
        DrawBitmap(embedder_for_roots_[view]);
      }
    } else if ((event->key_data &&
                event->key_data->windows_key_code == KEYBOARD_CODE_UP) ||
               (event->pointer_data &&
                event->pointer_data->vertical_wheel > 0)) {
      if (current_page_ > 0) {
        current_page_--;
        DrawBitmap(embedder_for_roots_[view]);
      }
    }
  }

  void OnViewDestroyed(View* view) override {
    DCHECK(embedder_for_roots_.find(view) != embedder_for_roots_.end());
    const auto& it = embedder_for_roots_.find(view);
    DCHECK(it != embedder_for_roots_.end());
    delete it->second;
    embedder_for_roots_.erase(it);
    if (embedder_for_roots_.size() == 0)
      ApplicationImpl::Terminate();
  }

  void DrawBitmap(EmbedderData* embedder_data) {
    if (!doc_)
      return;

    FPDF_PAGE page = FPDF_LoadPage(doc_, current_page_);
    int width = static_cast<int>(FPDF_GetPageWidth(page));
    int height = static_cast<int>(FPDF_GetPageHeight(page));

    scoped_ptr<std::vector<unsigned char>> bitmap;
    bitmap.reset(new std::vector<unsigned char>);
    bitmap->resize(width * height * 4);

    FPDF_BITMAP f_bitmap = FPDFBitmap_CreateEx(width, height, FPDFBitmap_BGRA,
                                               &(*bitmap)[0], width * 4);
    FPDFBitmap_FillRect(f_bitmap, 0, 0, width, height, 0xFFFFFFFF);
    FPDF_RenderPageBitmap(f_bitmap, page, 0, 0, width, height, 0, 0);
    FPDFBitmap_Destroy(f_bitmap);

    FPDF_ClosePage(page);

    embedder_data->bitmap_uploader().SetBitmap(width, height, bitmap.Pass(),
                                               BitmapUploader::BGRA);
  }

  void FetchPDF(URLResponsePtr response) {
    data_.clear();
    mojo::common::BlockingCopyToString(response->body.Pass(), &data_);
    if (data_.length() >= static_cast<size_t>(std::numeric_limits<int>::max()))
      return;
    doc_ = FPDF_LoadMemDocument(data_.data(), static_cast<int>(data_.length()),
                                nullptr);
    page_count_ = FPDF_GetPageCount(doc_);
  }

  std::string data_;
  int current_page_;
  int page_count_;
  FPDF_DOCUMENT doc_;
  ApplicationImpl* app_;
  std::map<View*, EmbedderData*> embedder_for_roots_;
  scoped_ptr<ViewManagerClientFactory> view_manager_client_factory_;

  DISALLOW_COPY_AND_ASSIGN(PDFView);
};

class PDFViewer : public ApplicationDelegate,
                  public ContentHandlerFactory::ManagedDelegate {
 public:
  PDFViewer() : content_handler_factory_(this) {
    v8::V8::InitializeICU();
    FPDF_InitLibrary();
  }

  ~PDFViewer() override { FPDF_DestroyLibrary(); }

 private:
  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(InterfaceRequest<Application> application_request,
                    URLResponsePtr response) override {
    return make_handled_factory_holder(new mojo::ApplicationImpl(
        new PDFView(response.Pass()), application_request.Pass()));
  }

  ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(PDFViewer);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::PDFViewer());
  return runner.Run(application_request);
}
