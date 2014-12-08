// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/test/js_application_test_base.h"
#include "services/js/test/pingpong_service.mojom.h"

namespace js {
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

struct PingTargetCallback {
  explicit PingTargetCallback(bool *b) : value(b) {}
  void Run(bool callback_value) const {
    *value = callback_value;
  }
  bool *value;
};

// Verify that "pingpong.js" implements the PingPongService interface
// and sends responses to our client.
TEST_F(JSPingPongTest, PingServiceToPongClient) {
  EXPECT_EQ(0, pingpong_client_.last_pong_value());
  pingpong_service_->Ping(1);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_EQ(2, pingpong_client_.last_pong_value());
  pingpong_service_->Ping(100);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_EQ(101, pingpong_client_.last_pong_value());
  pingpong_service_->Quit();
}

// Verify that "pingpong.js" can connect to "pingpong-target.js", act as
// its client, and return a Promise that only resolves after the target.ping()
// => client.pong() methods have executed 10 times.
TEST_F(JSPingPongTest, PingTargetApp) {
  bool returned_value = false;
  PingTargetCallback callback(&returned_value);
  pingpong_service_->PingTarget(JSAppURL("pingpong_target.js"), 10, callback);
  EXPECT_TRUE(pingpong_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(returned_value);
  pingpong_service_->Quit();
}

}  // namespace
}  // namespace js
