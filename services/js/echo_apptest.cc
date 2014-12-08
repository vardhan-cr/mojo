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

struct EchoStringCallback {
  explicit EchoStringCallback(String *s) : echo_value(s) {}
  void Run(const String& value) const {
    *echo_value = value;
  }
  String *echo_value;
};

TEST_F(JSEchoTest, EchoString) {
  String foo;
  EchoStringCallback callback(&foo);
  echo_service_->EchoString("foo", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("foo", foo);
  echo_service_->EchoString("quit", callback);
}

TEST_F(JSEchoTest, EchoEmptyString) {
  String empty;
  EchoStringCallback callback(&empty);
  echo_service_->EchoString("", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("", empty);
  echo_service_->EchoString("quit", callback);
}

TEST_F(JSEchoTest, EchoNullString) {
  String null;
  EchoStringCallback callback(&null);
  echo_service_->EchoString(nullptr, callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(null.is_null());
  echo_service_->EchoString("quit", callback);
}

} // namespace
}  // namespace js
