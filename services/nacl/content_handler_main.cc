// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
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
base::ScopedFILE TempFileForURL(mojo::URLLoaderPtr& url_loader,
                                const GURL& url) {
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
    return nullptr;
  }

  return mojo::common::BlockingCopyToTempFile(response->body.Pass());
}

NaClDesc* FileStreamToNaClDesc(FILE* file_stream) {
  // Get file handle
  int fd = fileno(file_stream);
  if (fd == -1) {
    LOG(FATAL) << "Could not open the stream pointer's file descriptor";
  }

  // These file descriptors must be dup'd, since NaClDesc takes ownership
  // of the file descriptor. The descriptor is still needed to close
  // the file stream.
  fd = dup(fd);
  if (fd == -1) {
    LOG(FATAL) << "Could not dup the file descriptor";
  }

  NaClDesc* desc = NaClDescCreateWithFilePathMetadata(fd, "");

  // Clean up.
  if (fclose(file_stream)) {
    LOG(FATAL) << "Failed to close temp file";
  }

  return desc;
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

    irt_fp_ = TempFileForURL(url_loader, irt_url);
    if (!irt_fp_) {
      LOG(FATAL) << "Could not acquire the IRT";
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

    // Acquire the nexe.
    base::ScopedFILE nexe_fp =
        mojo::common::BlockingCopyToTempFile(response->body.Pass());
    if (!nexe_fp) {
      LOG(FATAL) << "could not redirect nexe to temp file";
    }

    // Acquire the IRT.
    mojo::URLLoaderPtr url_loader = url_loader_.Pass();
    LoadIRT(url_loader);

    NaClDesc* irt_desc = FileStreamToNaClDesc(irt_fp_.release());
    NaClDesc* nexe_desc = FileStreamToNaClDesc(nexe_fp.release());

    // Run.
    int exit_code = mojo::LaunchNaCl(
        nexe_desc, irt_desc, 0, nullptr,
        application_request.PassMessagePipe().release().value());

    // Exits the process cleanly, does not return.
    mojo::NaClExit(exit_code);
    NOTREACHED();
  }

  mojo::ContentHandlerFactory content_handler_factory_;
  GURL url_;
  base::ScopedFILE irt_fp_;

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
