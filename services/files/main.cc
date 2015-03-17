// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/files/public/interfaces/files.mojom.h"
#include "services/files/files_impl.h"

namespace mojo {
namespace files {

class FilesApp : public ApplicationDelegate, public InterfaceFactory<Files> {
 public:
  FilesApp() {}
  ~FilesApp() override {}

 private:
  // |ApplicationDelegate| override:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<Files>(this);
    return true;
  }

  // |InterfaceFactory<Files>| implementation:
  void Create(ApplicationConnection* connection,
              InterfaceRequest<Files> request) override {
    new FilesImpl(connection, request.Pass());
  }

  DISALLOW_COPY_AND_ASSIGN(FilesApp);
};

}  // namespace files
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new mojo::files::FilesApp());
  return runner.Run(shell_handle);
}
