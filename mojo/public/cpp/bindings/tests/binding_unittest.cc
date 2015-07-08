// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/public/cpp/system/macros.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/public/interfaces/bindings/tests/sample_interfaces.mojom.h"
#include "mojo/public/interfaces/bindings/tests/sample_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace {

class BindingTest : public testing::Test {
 public:
  BindingTest() {}
  ~BindingTest() override {}

  RunLoop& loop() { return loop_; }

 private:
  Environment env_;
  RunLoop loop_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(BindingTest);
};

class ServiceImpl : public sample::Service {
 public:
  ServiceImpl() {}
  ~ServiceImpl() override {}

 private:
  // sample::Service implementation
  void Frobinate(sample::FooPtr foo,
                 BazOptions options,
                 sample::PortPtr port,
                 const FrobinateCallback& callback) override {
    callback.Run(1);
  }
  void GetPort(InterfaceRequest<sample::Port> port) override {}

  MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceImpl);
};

TEST_F(BindingTest, Close) {
  bool called = false;
  sample::ServicePtr ptr;
  auto request = GetProxy(&ptr);
  ptr.set_connection_error_handler([&called]() { called = true; });
  ServiceImpl impl;
  Binding<sample::Service> binding(&impl, request.Pass());

  binding.Close();
  EXPECT_FALSE(called);
  loop().RunUntilIdle();
  EXPECT_TRUE(called);
}

// Tests that destroying a mojo::Binding closes the bound message pipe handle.
TEST_F(BindingTest, DestroyClosesMessagePipe) {
  bool encountered_error = false;
  ServiceImpl impl;
  sample::ServicePtr ptr;
  auto request = GetProxy(&ptr);
  ptr.set_connection_error_handler(
      [&encountered_error]() { encountered_error = true; });
  bool called = false;
  auto called_cb = [&called](int32_t result) { called = true; };
  {
    Binding<sample::Service> binding(&impl, request.Pass());
    ptr->Frobinate(nullptr, sample::Service::BAZ_OPTIONS_REGULAR, nullptr,
                   called_cb);
    loop().RunUntilIdle();
    EXPECT_TRUE(called);
    EXPECT_FALSE(encountered_error);
  }
  // Now that the Binding is out of scope we should detect an error on the other
  // end of the pipe.
  loop().RunUntilIdle();
  EXPECT_TRUE(encountered_error);

  // And calls should fail.
  called = false;
  ptr->Frobinate(nullptr, sample::Service::BAZ_OPTIONS_REGULAR, nullptr,
                 called_cb);
  loop().RunUntilIdle();
  EXPECT_FALSE(called);
}

// Tests that the binding's connection error handler gets called when the other
// end is closed.
TEST_F(BindingTest, ConnectionError) {
  bool called = false;
  {
    ServiceImpl impl;
    sample::ServicePtr ptr;
    Binding<sample::Service> binding(&impl, GetProxy(&ptr));
    binding.set_connection_error_handler([&called]() { called = true; });
    ptr.reset();
    EXPECT_FALSE(called);
    loop().RunUntilIdle();
    EXPECT_TRUE(called);
    // We want to make sure that it isn't called again during destruction.
    called = false;
  }
  EXPECT_FALSE(called);
}

// Tests that calling Close doesn't result in the connection error handler being
// called.
TEST_F(BindingTest, CloseDoesntCallConnectionErrorHandler) {
  ServiceImpl impl;
  sample::ServicePtr ptr;
  Binding<sample::Service> binding(&impl, GetProxy(&ptr));
  bool called = false;
  binding.set_connection_error_handler([&called]() { called = true; });
  binding.Close();
  loop().RunUntilIdle();
  EXPECT_FALSE(called);

  // We can also close the other end, and the error handler still won't be
  // called.
  ptr.reset();
  loop().RunUntilIdle();
  EXPECT_FALSE(called);
}

class ServiceImplWithBinding : public ServiceImpl {
 public:
  ServiceImplWithBinding(InterfaceRequest<sample::Service> request,
                         bool* was_deleted)
      : binding_(this, request.Pass()), was_deleted_(was_deleted) {
    binding_.set_connection_error_handler([this]() { delete this; });
  }
  ~ServiceImplWithBinding() override { *was_deleted_ = true; }

 private:
  Binding<sample::Service> binding_;
  bool* const was_deleted_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ServiceImplWithBinding);
};

// Tests that the binding may be deleted in the connection error handler.
TEST_F(BindingTest, SelfDeleteOnConnectionError) {
  bool was_deleted = false;
  sample::ServicePtr ptr;
  // This should delete itself on connection error.
  new ServiceImplWithBinding(GetProxy(&ptr), &was_deleted);
  ptr.reset();
  EXPECT_FALSE(was_deleted);
  loop().RunUntilIdle();
  EXPECT_TRUE(was_deleted);
}

// Tests that explicitly calling Unbind followed by rebinding works.
TEST_F(BindingTest, Unbind) {
  ServiceImpl impl;
  sample::ServicePtr ptr;
  Binding<sample::Service> binding(&impl, GetProxy(&ptr));

  bool called = false;
  auto called_cb = [&called](int32_t result) { called = true; };
  ptr->Frobinate(nullptr, sample::Service::BAZ_OPTIONS_REGULAR, nullptr,
                 called_cb);
  loop().RunUntilIdle();
  EXPECT_TRUE(called);

  called = false;
  auto request = binding.Unbind();
  EXPECT_FALSE(binding.is_bound());
  // All calls should fail when not bound...
  ptr->Frobinate(nullptr, sample::Service::BAZ_OPTIONS_REGULAR, nullptr,
                 called_cb);
  loop().RunUntilIdle();
  EXPECT_FALSE(called);

  called = false;
  binding.Bind(request.Pass());
  EXPECT_TRUE(binding.is_bound());
  // ...and should succeed again when the rebound.
  ptr->Frobinate(nullptr, sample::Service::BAZ_OPTIONS_REGULAR, nullptr,
                 called_cb);
  loop().RunUntilIdle();
  EXPECT_TRUE(called);
}

class IntegerAccessorImpl : public sample::IntegerAccessor {
 public:
  IntegerAccessorImpl() {}
  ~IntegerAccessorImpl() override {}

 private:
  // sample::IntegerAccessor implementation.
  void GetInteger(const GetIntegerCallback& callback) override {
    callback.Run(1, sample::ENUM_VALUE);
  }
  void SetInteger(int64_t data, sample::Enum type) override {}

  MOJO_DISALLOW_COPY_AND_ASSIGN(IntegerAccessorImpl);
};

TEST_F(BindingTest, SetInterfacePtrVersion) {
  IntegerAccessorImpl impl;
  sample::IntegerAccessorPtr ptr;
  Binding<sample::IntegerAccessor> binding(&impl, &ptr);
  EXPECT_EQ(3u, ptr.version());
}

}  // namespace
}  // mojo
