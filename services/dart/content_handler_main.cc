// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/trace_event.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/dart/embedder/dart_controller.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/tracing/public/interfaces/tracing.mojom.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"
#include "services/dart/content_handler_app_service_connector.h"
#include "services/dart/dart_app.h"
#include "services/dart/dart_tracing.h"
#include "url/gurl.h"

namespace dart {

static bool IsDartZip(std::string url) {
  // If the url doesn't end with ".dart" we assume it is a zipped up
  // dart application.
  return !EndsWith(url, ".dart", false);
}

class DartContentHandlerApp;

class DartContentHandler : public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  DartContentHandler(DartContentHandlerApp* app, bool strict)
      : app_(app), strict_(strict) {
  }

  void set_handler_task_runner(
      scoped_refptr<base::SingleThreadTaskRunner> handler_task_runner) {
    handler_task_runner_ = handler_task_runner;
  }

 private:
  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override;

  DartContentHandlerApp* app_;
  bool strict_;
  scoped_refptr<base::SingleThreadTaskRunner> handler_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandler);
};

class DartContentHandlerApp : public mojo::ApplicationDelegate {
 public:
  DartContentHandlerApp()
      : content_handler_(this, false),
        strict_content_handler_(this, true),
        content_handler_factory_(&content_handler_),
        strict_content_handler_factory_(&strict_content_handler_),
        service_connector_(nullptr) {}

  ~DartContentHandlerApp() override {}

  void ExtractApplication(base::FilePath* application_dir,
                          mojo::URLResponsePtr response,
                          const base::Closure& callback) {
    url_response_disk_cache_->UpdateAndGetExtracted(
        response.Pass(),
        [application_dir, callback](mojo::Array<uint8_t> application_dir_path,
                                    mojo::Array<uint8_t> cache_path) {
          if (application_dir_path.is_null()) {
            *application_dir = base::FilePath();
          } else {
            *application_dir = base::FilePath(std::string(
                reinterpret_cast<char*>(&application_dir_path.front()),
                application_dir_path.size()));
          }
          callback.Run();
        });
  }

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    // Tracing of content handler and controller.
    tracing_.Initialize(app);
    // Tracing of isolates and VM.
    dart_tracing_.Initialize(app);

    // TODO(qsr): This has no effect for now, as the tracing infrastructure
    // doesn't allow to trace anything before the tracing app connects to the
    // application.
    TRACE_EVENT0("dart_content_handler", "DartContentHandler::Initialize");

    content_handler_.set_handler_task_runner(
        base::MessageLoop::current()->task_runner());
    strict_content_handler_.set_handler_task_runner(
        base::MessageLoop::current()->task_runner());
    app->ConnectToService("mojo:url_response_disk_cache",
                          &url_response_disk_cache_);
    service_connector_ = new ContentHandlerAppServiceConnector(app);
    bool success =
        mojo::dart::DartController::Initialize(service_connector_, false);
    if (!success) {
      LOG(ERROR) << "Dart VM Initialization failed";
    }
  }

  bool HasStrictQueryParam(const std::string& requestedUrl) {
    bool strict_compilation = false;
    GURL url(requestedUrl);
    if (url.has_query()) {
      std::vector<std::string> query_parameters;
      Tokenize(url.query(), "&", &query_parameters);
      strict_compilation =
          std::find(query_parameters.begin(), query_parameters.end(),
                    "strict=true") != query_parameters.end();
    }
    return strict_compilation;
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    bool strict = HasStrictQueryParam(connection->GetConnectionURL());
    if (strict) {
      connection->AddService(&strict_content_handler_factory_);
    } else {
      connection->AddService(&content_handler_factory_);
    }
    return true;
  }

  // Overridden from ApplicationDelegate:
  void Quit() override {
    // Shutdown the controller.
    mojo::dart::DartController::Shutdown();
    delete service_connector_;
  }

  mojo::TracingImpl tracing_;
  DartContentHandler content_handler_;
  DartContentHandler strict_content_handler_;
  mojo::ContentHandlerFactory content_handler_factory_;
  mojo::ContentHandlerFactory strict_content_handler_factory_;
  mojo::URLResponseDiskCachePtr url_response_disk_cache_;
  ContentHandlerAppServiceConnector* service_connector_;
  DartTracingImpl dart_tracing_;

  DISALLOW_COPY_AND_ASSIGN(DartContentHandlerApp);
};

scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
DartContentHandler::CreateApplication(
    mojo::InterfaceRequest<mojo::Application> application_request,
    mojo::URLResponsePtr response) {
  base::trace_event::TraceLog::GetInstance()
      ->SetCurrentThreadBlocksMessageLoop();

  TRACE_EVENT1("dart_content_handler", "DartContentHandler::CreateApplication",
               "url", response->url.get());

  base::FilePath application_dir;
  std::string url = response->url.get();
  if (IsDartZip(response->url.get())) {
    // Loading a zipped snapshot:
    // 1) Extract the zip file.
    // 2) Launch from temporary directory (|application_dir|).
    handler_task_runner_->PostTask(
        FROM_HERE,
        base::Bind(
            &DartContentHandlerApp::ExtractApplication, base::Unretained(app_),
            base::Unretained(&application_dir), base::Passed(response.Pass()),
            base::Bind(
                base::IgnoreResult(&base::SingleThreadTaskRunner::PostTask),
                base::MessageLoop::current()->task_runner(), FROM_HERE,
                base::MessageLoop::QuitWhenIdleClosure())));
    base::RunLoop().Run();
    return make_scoped_ptr(
        new DartApp(application_request.Pass(), application_dir, strict_));
  } else {
    // Loading a raw .dart file pointed at by |url|.
    return make_scoped_ptr(
        new DartApp(application_request.Pass(), url, strict_));
  }
}

}  // namespace dart

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new dart::DartContentHandlerApp);
  MojoResult r = runner.Run(application_request);
  // TODO(johnmccutchan): Remove this once the Dart VM shuts down threads.
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(100));
  return r;
}
