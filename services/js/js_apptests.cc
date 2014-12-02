// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "services/js/test/echo_service.mojom.h"

namespace mojo {
namespace {

class JSEchoTest : public test::ApplicationTestBase {
 public:
  JSEchoTest() : ApplicationTestBase() {}
  ~JSEchoTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    application_impl()->ConnectToService(service_path(), &echo_service_);
  }

  EchoServicePtr echo_service_;

 private:
  const std::string service_path() {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("services")
        .AppendASCII("js")
        .AppendASCII("test")
        .AppendASCII("echo.js");
    return "file://" + path.AsUTF8Unsafe();
  }

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

}  // namespace
}  // namespace mojo
