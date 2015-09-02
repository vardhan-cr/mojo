// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>

#include "base/files/file_util.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/nacl/irt_mojo_nonsfi.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"

namespace nacl {
namespace content_handler {
namespace {

// Copies response (input) into new temporary file open by fd (output).
bool URLResponseToTempFile(mojo::URLResponsePtr& response, int* fd) {
  base::FilePath path;
  if (!base::CreateTemporaryFile(&path)) {
    return false;
  }

  if (!mojo::common::BlockingCopyToFile(response->body.Pass(), path)) {
    base::DeleteFile(path, false);
    return false;
  }
  *fd = open(path.value().c_str(), O_RDONLY);
  if (*fd < 0) {
    LOG(FATAL) << "Failed to open " << path.value().c_str() << ": "
               << strerror(errno) << "\n";
  }

  // Since we unlink an open file, the temp file will be removed as soon as the
  // fd is closed.
  if (unlink(path.value().c_str())) {
    LOG(FATAL) << "Failed to unlink " << path.value().c_str() << ": "
               << strerror(errno) << "\n";
  }
  return true;
}

}  // namespace

class NaClContentHandler : public mojo::ApplicationDelegate,
                           public mojo::ContentHandlerFactory::Delegate {
 public:
  NaClContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {}

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
    int fd;
    if (!URLResponseToTempFile(response, &fd)) {
      LOG(FATAL) << "could not redirect nexe to temp file";
    }

    // Run -- also, closes the fd, removing the temp file.
    uintptr_t entry = NaClLoadElfFile(fd);

    irtNonsfi::MojoSetInitialHandle(
        application_request.PassMessagePipe().release().value());
    int argc = 1;
    char* argvp = const_cast<char*>("NaClMain");
    char* envp = nullptr;
    nacl_irt_nonsfi_entry(argc, &argvp, &envp,
                          reinterpret_cast<nacl_entry_func_t>(entry),
                          irtNonsfi::MojoIrtNonsfiQuery);
    abort();
    NOTREACHED();
  }

  mojo::ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(NaClContentHandler);
};

}  // namespace content_handler
}  // namespace nacl

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new nacl::content_handler::NaClContentHandler());
  return runner.Run(application_request);
}
