// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "services/http_server/public/http_server.mojom.h"
#include "services/http_server/public/http_server_factory.mojom.h"
#include "services/http_server/public/http_server_util.h"

namespace http_server {

class TestHttpHandler : public http_server::HttpHandler {
 public:
  TestHttpHandler(mojo::InterfaceRequest<HttpHandler> request)
      : binding_(this, request.Pass()) {}
  ~TestHttpHandler() override {}

  void AddHandlerCallback(bool result) { CHECK(result); }

  bool WaitForIncomingMethodCall() {
    return binding_.WaitForIncomingMethodCall();
  }

 private:
  // http_server::HttpHandler:
  void HandleRequest(http_server::HttpRequestPtr request,
                     const mojo::Callback<void(http_server::HttpResponsePtr)>&
                         callback) override {
    callback.Run(http_server::CreateHttpResponse(200, "Hello World\n"));
  }

  mojo::Binding<http_server::HttpHandler> binding_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(TestHttpHandler);
};

class HttpServerApplicationTest : public mojo::test::ApplicationTestBase {
 public:
  HttpServerApplicationTest() : ApplicationTestBase() {}
  ~HttpServerApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    application_impl()->ConnectToService("mojo:http_server",
                                         &http_server_factory_);
    application_impl()->ConnectToService("mojo:network_service",
                                         &network_service_);
  }

  http_server::HttpServerFactoryPtr http_server_factory_;
  mojo::NetworkServicePtr network_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(HttpServerApplicationTest);
};

void CheckServerResponse(mojo::URLResponsePtr response) {
  EXPECT_EQ(200u, response->status_code);
  std::string response_body;
  mojo::common::BlockingCopyToString(response->body.Pass(), &response_body);
  EXPECT_EQ("Hello World\n", response_body);
  base::MessageLoop::current()->Quit();
}

TEST_F(HttpServerApplicationTest, ServerResponse) {
  http_server::HttpServerPtr http_server;
  http_server_factory_->CreateHttpServer(GetProxy(&http_server).Pass(), 0u);

  uint16_t assigned_port;
  http_server->GetPort([&assigned_port](uint16_t p) { assigned_port = p; });
  http_server.WaitForIncomingMethodCall();

  HttpHandlerPtr http_handler_ptr;
  TestHttpHandler handler(GetProxy(&http_handler_ptr).Pass());

  // Set the test handler and wait for confirmation.
  http_server->SetHandler("/test", http_handler_ptr.Pass(),
                          base::Bind(&TestHttpHandler::AddHandlerCallback,
                                     base::Unretained(&handler)));
  http_server.WaitForIncomingMethodCall();

  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://0.0.0.0:%u/test", assigned_port);
  url_loader->Start(url_request.Pass(), base::Bind(&CheckServerResponse));
  base::RunLoop run_loop;
  run_loop.Run();
}

}  // namespace http_server
