// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/test/js_application_test_base.h"
#include "services/js/test/pingpong_service.mojom.h"

namespace mojo {
namespace {

class PingPongClientImpl : public PingPongClient {
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

class JSPingPongTest : public test::JSApplicationTestBase {
 public:
  JSPingPongTest() : JSApplicationTestBase() {}
  ~JSPingPongTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = JSAppURL("pingpong.js");
    application_impl()->ConnectToService(url, &pingpong_service_);
    pingpong_service_.set_client(&pingpong_client_);
  }

  PingPongServicePtr pingpong_service_;
  PingPongClientImpl pingpong_client_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(JSPingPongTest);
};

TEST_F(JSPingPongTest, PingServiceToPongClient) {
  EXPECT_EQ(0, pingpong_client_.last_pong_value());
  pingpong_service_->Ping(1);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_EQ(2, pingpong_client_.last_pong_value());
  pingpong_service_->Ping(100);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_EQ(101, pingpong_client_.last_pong_value());
  pingpong_service_->Ping(999); // Ask the service to quit.
}

}  // namespace
}  // namespace mojo
