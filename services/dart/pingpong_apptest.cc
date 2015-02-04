// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "services/dart/test/pingpong_service.mojom.h"

namespace dart {
namespace {

class PingPongClientImpl : public mojo::InterfaceImpl<PingPongClient> {
 public:
  PingPongClientImpl() : last_pong_value_(0) {};
  ~PingPongClientImpl() override {};

  int16_t last_pong_value() const { return last_pong_value_; }

  // PingPongClient overrides.
  void Pong(uint16_t pong_value) override {
    last_pong_value_ = pong_value;
  }

 private:
  int16_t last_pong_value_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(PingPongClientImpl);
};

class DartPingPongTest : public mojo::test::ApplicationTestBase {
 public:
  DartPingPongTest() : mojo::test::ApplicationTestBase() {}
  ~DartPingPongTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:dart_pingpong",
                                         &pingpong_service_);
  }

  PingPongServicePtr pingpong_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(DartPingPongTest);
};

struct PingTargetCallback {
  explicit PingTargetCallback(bool *b) : value(b) {}
  void Run(bool callback_value) const {
    *value = callback_value;
  }
  bool *value;
};

// Verify that "pingpong.dart" implements the PingPongService interface
// and sends responses to our client.
TEST_F(DartPingPongTest, PingServiceToPongClient) {
  PingPongClientPtr client;
  PingPongClientImpl* pingpong_client = new PingPongClientImpl();
  BindToProxy(pingpong_client, &client);
  pingpong_service_->SetClient(client.Pass());

  EXPECT_EQ(0, pingpong_client->last_pong_value());
  pingpong_service_->Ping(1);
  EXPECT_TRUE(pingpong_client->WaitForIncomingMethodCall());
  EXPECT_EQ(2, pingpong_client->last_pong_value());
  pingpong_service_->Ping(100);
  EXPECT_TRUE(pingpong_client->WaitForIncomingMethodCall());
  EXPECT_EQ(101, pingpong_client->last_pong_value());
  pingpong_service_->Quit();
}

// Verify that "pingpong.dart" can connect to "pingpong_target.dart", act as
// its client, and return a Future that only resolves after the target.ping()
// => client.pong() methods have executed 9 times.
TEST_F(DartPingPongTest, PingTargetURL) {
  bool returned_value = false;
  PingTargetCallback callback(&returned_value);
  pingpong_service_->PingTargetURL("mojo:dart_pingpong_target", 9, callback);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(returned_value);
  pingpong_service_->Quit();
}

// Same as the previous test except that instead of providing the
// pingpong_target.dart URL, we provide a connection to its PingPongService.
TEST_F(DartPingPongTest, PingTargetService) {
  PingPongServicePtr target;
  application_impl()->ConnectToService("mojo:dart_pingpong_target", &target);
  bool returned_value = false;
  PingTargetCallback callback(&returned_value);
  pingpong_service_->PingTargetService(target.Pass(), 9, callback);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(returned_value);
  pingpong_service_->Quit();
}

// Verify that Dart can implement an interface "request" parameter.
TEST_F(DartPingPongTest, GetTargetService) {
  PingPongServicePtr target;
  PingPongClientPtr client;
  PingPongClientImpl* pingpong_client = new PingPongClientImpl();
  BindToProxy(pingpong_client, &client);

  pingpong_service_->GetPingPongService(GetProxy(&target));
  target->SetClient(client.Pass());

  EXPECT_EQ(0, pingpong_client->last_pong_value());
  target->Ping(1);
  EXPECT_TRUE(pingpong_client->WaitForIncomingMethodCall());
  EXPECT_EQ(2, pingpong_client->last_pong_value());
  target->Ping(100);
  EXPECT_TRUE(pingpong_client->WaitForIncomingMethodCall());
  EXPECT_EQ(101, pingpong_client->last_pong_value());
  target->Quit();
  pingpong_service_->Quit();
}

}  // namespace
}  // namespace dart
