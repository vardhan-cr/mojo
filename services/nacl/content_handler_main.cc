// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "build/build_config.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "nacl_bindings/monacl_sel_main.h"
#include "native_client/src/public/nacl_desc.h"
#include "url/gurl.h"

namespace nacl {
namespace content_handler {

namespace {
bool URLResponseToTempFile(mojo::URLResponsePtr& response,
                           base::FilePath* file_path) {
  if (!base::CreateTemporaryFile(file_path)) {
    return false;
  }

  if (!mojo::common::BlockingCopyToFile(response->body.Pass(), *file_path)) {
    base::DeleteFile(*file_path, false);
    return false;
  }

  // TODO(ncbray): can we ensure temp file deletion even if we crash?
  return true;
}

bool TempFileForURL(mojo::URLLoaderPtr& url_loader,
                    const GURL& url,
                    base::FilePath* path) {
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = url.spec();
  request->method = "GET";
  request->auto_follow_redirects = true;

  // Handle the callback synchonously.
  mojo::URLResponsePtr response;
  url_loader->Start(request.Pass(), [&response](mojo::URLResponsePtr r) {
    response = r.Pass();
  });
  url_loader.WaitForIncomingResponse();
  if (response.is_null()) {
    LOG(FATAL) << "something went horribly wrong (missed a callback?)";
  }

  if (response->error) {
    LOG(ERROR) << "could not load " << url.spec() << " ("
               << response->error->code << ") "
               << response->error->description.get().c_str();
    return false;
  }

  return URLResponseToTempFile(response, path);
}

NaClDesc* NaClDescFromNexePath(base::FilePath& path) {
  base::File file(path, base::File::FLAG_OPEN | base::File::FLAG_READ |
                            base::File::FLAG_EXECUTE);
  if (!file.IsValid()) {
    LOG(FATAL) << "failed to open " << path.value();
  }
  // Note: potentially unsafe, assumes the file is immutable. This should be
  // OK because we're currently only using temp files.
  return NaClDescCreateWithFilePathMetadata(file.TakePlatformFile(),
                                            path.AsUTF8Unsafe().c_str());
}

}  // namespace

class NaClContentHandler : public mojo::ApplicationDelegate,
                           public mojo::ContentHandlerFactory::Delegate {
 public:
  NaClContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    url_ = GURL(app->url());

    mojo::NetworkServicePtr network_service;
    app->ConnectToService("mojo:network_service", &network_service);

    network_service->CreateURLLoader(GetProxy(&url_loader_));
  }

  void LoadIRT(mojo::URLLoaderPtr& url_loader) {
    // TODO(ncbray): support other architectures.
    GURL irt_url;
#if defined(ARCH_CPU_X86_64)
    irt_url = url_.Resolve("irt_x64/irt_mojo.nexe");
#else
#error "Unknown / unsupported CPU architecture."
#endif
    if (!irt_url.is_valid()) {
      LOG(FATAL) << "Cannot resolve IRT URL";
    }

    if (!TempFileForURL(url_loader, irt_url, &irt_path_)) {
      LOG(FATAL) << "Could not aquire the IRT";
    }
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  void RunApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    // Needed to use Mojo interfaces on this thread.
    base::MessageLoop loop(mojo::common::MessagePumpMojo::Create());

    // Aquire the nexe.
    base::FilePath nexe_path;
    if (!URLResponseToTempFile(response, &nexe_path)) {
      LOG(FATAL) << "could not redirect nexe to temp file";
    }

    // Aquire the IRT.
    mojo::URLLoaderPtr url_loader = url_loader_.Pass();
    LoadIRT(url_loader);

    // Get file handles to the IRT and nexe.
    NaClDesc* irt_desc = NaClDescFromNexePath(irt_path_);
    NaClDesc* nexe_desc = NaClDescFromNexePath(nexe_path);

    // Run.
    int exit_code = mojo::LaunchNaCl(
        nexe_desc, irt_desc, 0, NULL,
        application_request.PassMessagePipe().release().value());

    // TODO(ncbray): are the file handles actually closed at this point?

    // Clean up.
    if (!base::DeleteFile(irt_path_, false)) {
      LOG(FATAL) << "Failed to remove irt temp file " << irt_path_.value();
    }
    if (!base::DeleteFile(nexe_path, false)) {
      LOG(FATAL) << "Failed to remove nexe temp file " << nexe_path.value();
    }

    // Exits the process cleanly, does not return.
    mojo::NaClExit(exit_code);
    NOTREACHED();
  }

  mojo::ContentHandlerFactory content_handler_factory_;
  GURL url_;
  base::FilePath irt_path_;

  mojo::URLLoaderPtr url_loader_;

  DISALLOW_COPY_AND_ASSIGN(NaClContentHandler);
};

}  // namespace content_handler
}  // namespace nacl

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new nacl::content_handler::NaClContentHandler());
  return runner.Run(application_request);
}
