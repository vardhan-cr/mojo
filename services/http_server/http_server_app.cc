// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "services/http_server/http_server_impl.h"
#include "services/http_server/public/http_server.mojom.h"

namespace http_server {

class HttpServerApp : public mojo::ApplicationDelegate,
                      public mojo::InterfaceFactory<HttpServer> {
 public:
  HttpServerApp() {}

  virtual void Initialize(mojo::ApplicationImpl* app) override { app_ = app; }

 private:
  // ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

  // InterfaceFactory<HttpServer>:
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<HttpServer> request) override {
    if (!http_server_) {
      http_server_.reset(new HttpServerImpl(app_));
    }
    http_server_->AddBinding(request.Pass());
  }

  mojo::ApplicationImpl* app_;
  scoped_ptr<HttpServerImpl> http_server_;
};

}  // namespace http_server

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunner runner(new http_server::HttpServerApp);
  return runner.Run(shell_handle);
}
