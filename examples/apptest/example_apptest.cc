// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/apptest/example_service.mojom.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/environment/logging.h"
#include "mojo/public/cpp/system/macros.h"

namespace mojo {
namespace {
// Exemplifies ApplicationTestBase's application testing pattern.
class ExampleApplicationTest : public test::ApplicationTestBase {
 public:
  ExampleApplicationTest() : ApplicationTestBase() {}
  ~ExampleApplicationTest() override {}

 protected:
  // ApplicationTestBase:
  void SetUp() override {
    ApplicationTestBase::SetUp();

    application_impl()->ConnectToService("mojo:example_service",
                                         &example_service_);
  }

  ExampleServicePtr example_service_;

 private:
  MOJO_DISALLOW_COPY_AND_ASSIGN(ExampleApplicationTest);
};

struct PongCallback {
  explicit PongCallback(uint16_t* pong_value) : pong_value(pong_value) {}

  void Run(uint16_t value) const { *pong_value = value; }

  uint16_t* pong_value;
};

TEST_F(ExampleApplicationTest, PingServiceToPong) {
  uint16_t pong_value = 0u;
  example_service_->Ping(1u, PongCallback(&pong_value));
  EXPECT_TRUE(example_service_.WaitForIncomingMethodCall());
  // Test making a call and receiving the reply.
  EXPECT_EQ(1u, pong_value);
}

TEST_F(ExampleApplicationTest, CheckCommandLineArg) {
  // apptest_runner.py adds this argument unconditionally, so we can check for
  // it here to verify that command line args are getting passed to apptests.
  ASSERT_TRUE(application_impl()->HasArg("--example_apptest_arg"));
}

}  // namespace
}  // namespace mojo
