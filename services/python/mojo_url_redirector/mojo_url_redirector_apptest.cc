// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>

#include "base/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/services/network/public/interfaces/net_address.mojom.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"

namespace mojo_url_redirector {

class MojoUrlRedirectorApplicationTest :
    public mojo::test::ApplicationTestBase {
 public:
  MojoUrlRedirectorApplicationTest() : ApplicationTestBase(),
      assigned_port_(0), handler_registered_(false) {}
  ~MojoUrlRedirectorApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    // Construct the server's address.
    mojo::NetAddressPtr server_address(mojo::NetAddress::New());
    server_address->family = mojo::NET_ADDRESS_FAMILY_IPV4;
    server_address->ipv4 = mojo::NetAddressIPv4::New();
    server_address->ipv4->addr.resize(4);
    for (const std::string& arg : application_impl()->args()) {
      if (arg.find("--address") != std::string::npos) {
        sscanf(arg.c_str(), "--address=%hhu.%hhu.%hhu.%hhu:%hu",
               &server_address->ipv4->addr[0],
               &server_address->ipv4->addr[1],
               &server_address->ipv4->addr[2],
               &server_address->ipv4->addr[3],
               &assigned_port_);
        server_address->ipv4->port = assigned_port_;
        break;
      }
    }
    DCHECK(assigned_port_);

    application_impl()->ConnectToApplication("mojo:mojo_url_redirector");
    application_impl()->ConnectToService("mojo:network_service",
                                         &network_service_);

    // Wait until the MojoUrlRedirector is registered as a handler with the
    // server.
    WaitForHandlerRegistration();
  }

  mojo::NetworkServicePtr network_service_;
  uint16_t assigned_port_;
  bool handler_registered_;

 private:
  void WaitForHandlerRegistration();
  void CheckHandlerRegistered(mojo::URLResponsePtr response);

  MOJO_DISALLOW_COPY_AND_ASSIGN(MojoUrlRedirectorApplicationTest);
};

void MojoUrlRedirectorApplicationTest::WaitForHandlerRegistration() {
  while (!handler_registered_) {
    mojo::URLLoaderPtr url_loader;
    network_service_->CreateURLLoader(GetProxy(&url_loader));
    mojo::URLRequestPtr url_request = mojo::URLRequest::New();
    url_request->url =
        base::StringPrintf("http://localhost:%u/test", assigned_port_);
    url_loader->Start(url_request.Pass(),
        base::Bind(&MojoUrlRedirectorApplicationTest::CheckHandlerRegistered,
                   base::Unretained(this)));
    ASSERT_TRUE(url_loader.WaitForIncomingMethodCall());
  }
}

void MojoUrlRedirectorApplicationTest::CheckHandlerRegistered(
    mojo::URLResponsePtr response) {
  if (response->error) {
    // The server has not yet been spun up.
    return;
  }

  if (response->status_code == 404) {
    // The handler has not yet been registered.
    return;
  }

  handler_registered_ = true;
}

void CheckHandlerResponse(uint32 expected_http_status,
                         mojo::URLResponsePtr response) {
  EXPECT_EQ(nullptr, response->error);
  EXPECT_EQ(expected_http_status, response->status_code);
  std::string response_body;
  mojo::common::BlockingCopyToString(response->body.Pass(), &response_body);
  EXPECT_EQ("", response_body);
}

TEST_F(MojoUrlRedirectorApplicationTest, MalformedRequest) {
  mojo::URLLoaderPtr url_loader;
  network_service_->CreateURLLoader(GetProxy(&url_loader));

  mojo::URLRequestPtr url_request = mojo::URLRequest::New();
  url_request->url =
      base::StringPrintf("http://localhost:%u/test", assigned_port_);
  url_loader->Start(url_request.Pass(), base::Bind(&CheckHandlerResponse,
                                                   400));
  url_loader.WaitForIncomingMethodCall();
}

}  // namespace mojo_url_redirector
