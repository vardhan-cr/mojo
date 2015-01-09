// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/test/echo_service.mojom.h"
#include "services/js/test/js_application_test_base.h"

using mojo::String;

namespace js {
namespace {

class JSEchoTest : public test::JSApplicationTestBase {
 public:
  JSEchoTest() : JSApplicationTestBase() {}
  ~JSEchoTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = JSAppURL("echo.js");
    application_impl()->ConnectToService(url, &echo_service_);
  }

  mojo::EchoServicePtr echo_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(JSEchoTest);
};

class JSServiceProviderEchoTest : public test::JSApplicationTestBase {
 public:
  JSServiceProviderEchoTest() : JSApplicationTestBase() {}
  ~JSServiceProviderEchoTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = JSAppURL("echo.js");
    application_impl()->shell()->ConnectToApplication(
        url, GetProxy(&echo_service_provider_));
  }

  mojo::ServiceProviderPtr echo_service_provider_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(JSServiceProviderEchoTest);
};

struct EchoStringCallback {
  explicit EchoStringCallback(String *s) : echo_value(s) {}
  void Run(const String& value) const {
    *echo_value = value;
  }
  String *echo_value;
};

struct ShareEchoServiceCallback {
  explicit ShareEchoServiceCallback(bool *b) : value(b) {}
  void Run(bool callback_value) const {
    *value = callback_value;
  }
  bool *value;
};

TEST_F(JSEchoTest, EchoString) {
  String foo;
  EchoStringCallback callback(&foo);
  echo_service_->EchoString("foo", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("foo", foo);
  echo_service_->Quit();
}

TEST_F(JSEchoTest, EchoEmptyString) {
  String empty;
  EchoStringCallback callback(&empty);
  echo_service_->EchoString("", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("", empty);
  echo_service_->Quit();
}

TEST_F(JSEchoTest, EchoNullString) {
  String null;
  EchoStringCallback callback(&null);
  echo_service_->EchoString(nullptr, callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(null.is_null());
  echo_service_->Quit();
}

// Verify that a JS app's ServiceProvider can request and provide services.
// This test exercises the same code paths as examples/js/share_echo.js.
TEST_F(JSEchoTest, ShareEchoService) {
  bool returned_value;
  ShareEchoServiceCallback callback(&returned_value);
  echo_service_->ShareEchoService(callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(returned_value);
  echo_service_->Quit();
}

// Verify that connecting via the echo service application's ServiceProvider
// behaves the same as connecting to the echo service directly.
TEST_F(JSServiceProviderEchoTest, UseApplicationServiceProvider) {
  mojo::EchoServicePtr echo_service;
  mojo::MessagePipe pipe;
  echo_service.Bind(pipe.handle0.Pass());
  echo_service_provider_->ConnectToService(
      mojo::EchoService::Name_, pipe.handle1.Pass());
  String foo;
  EchoStringCallback callback(&foo);
  echo_service->EchoString("foo", callback);
  EXPECT_TRUE(echo_service.WaitForIncomingMethodCall());
  EXPECT_EQ("foo", foo);
  echo_service->Quit();
}

} // namespace
} // namespace js
