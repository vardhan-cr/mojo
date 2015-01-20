// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "services/dart/test/echo_service.mojom.h"

using mojo::String;

namespace dart {
namespace {

class DartEchoTest : public mojo::test::ApplicationTestBase {
 public:
  DartEchoTest() : ApplicationTestBase() {}
  ~DartEchoTest() override {}

 protected:
  const std::string DartAppURL(const std::string& filename) {
    base::FilePath path;
    PathService::Get(base::DIR_SOURCE_ROOT, &path);
    path = path.AppendASCII("services")
        .AppendASCII("dart")
        .AppendASCII("test")
        .AppendASCII(filename);
    return "file://" + path.AsUTF8Unsafe();
  }

  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();
    const std::string& url = DartAppURL("echo.dart");
    application_impl()->ConnectToService(url, &echo_service_);
  }

  mojo::EchoServicePtr echo_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(DartEchoTest);
};

struct EchoStringCallback {
  explicit EchoStringCallback(String *s) : echo_value(s) {}
  void Run(const String& value) const {
    *echo_value = value;
  }
  String *echo_value;
};

TEST_F(DartEchoTest, EchoString) {
  String foo;
  EchoStringCallback callback(&foo);
  echo_service_->EchoString("foo", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("foo", foo);
  echo_service_->EchoString("quit", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("quit", foo);
}

TEST_F(DartEchoTest, EchoEmptyString) {
  String empty;
  EchoStringCallback callback(&empty);
  echo_service_->EchoString("", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("", empty);
  echo_service_->EchoString("quit", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("quit", empty);
}

TEST_F(DartEchoTest, EchoNullString) {
  String null;
  EchoStringCallback callback(&null);
  echo_service_->EchoString(nullptr, callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_TRUE(null.is_null());
  echo_service_->EchoString("quit", callback);
  EXPECT_TRUE(echo_service_.WaitForIncomingMethodCall());
  EXPECT_EQ("quit", null);
}

} // namespace
}  // namespace dart
