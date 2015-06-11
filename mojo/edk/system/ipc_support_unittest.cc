// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/ipc_support.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/synchronization/waitable_event.h"
#include "base/test/test_io_thread.h"
#include "base/test/test_timeouts.h"
#include "mojo/edk/embedder/master_process_delegate.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/edk/system/connection_identifier.h"
#include "mojo/edk/system/process_identifier.h"
#include "mojo/edk/test/multiprocess_test_helper.h"
#include "mojo/edk/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace system {
namespace {

const char kConnectionIdFlag[] = "test-connection-id";

class TestMasterProcessDelegate : public embedder::MasterProcessDelegate {
 public:
  TestMasterProcessDelegate()
      : on_slave_disconnect_event_(true, false) {}  // Manual reset.
  ~TestMasterProcessDelegate() override {}

  bool TryWaitForOnSlaveDisconnect() {
    return on_slave_disconnect_event_.TimedWait(TestTimeouts::action_timeout());
  }

 private:
  // |embedder::MasterProcessDelegate| methods:
  void OnShutdownComplete() override { NOTREACHED(); }

  void OnSlaveDisconnect(embedder::SlaveInfo /*slave_info*/) override {
    on_slave_disconnect_event_.Signal();
  }

  base::WaitableEvent on_slave_disconnect_event_;

  DISALLOW_COPY_AND_ASSIGN(TestMasterProcessDelegate);
};

class TestSlaveProcessDelegate : public embedder::SlaveProcessDelegate {
 public:
  TestSlaveProcessDelegate() {}
  ~TestSlaveProcessDelegate() override {}

 private:
  // |embedder::SlaveProcessDelegate| methods:
  void OnShutdownComplete() override { NOTREACHED(); }

  void OnMasterDisconnect() override { NOTREACHED(); }

  DISALLOW_COPY_AND_ASSIGN(TestSlaveProcessDelegate);
};

}  // namespace

// Note: This test isn't in an anonymous namespace, since it needs to be
// friended by |IPCSupport|.
#if defined(OS_ANDROID)
// Android multi-process tests are not executing the new process. This is flaky.
// TODO(vtl): I'm guessing this is true of this test too?
#define MAYBE_MultiprocessMasterSlaveInternal \
  DISABLED_MultiprocessMasterSlaveInternal
#else
#define MAYBE_MultiprocessMasterSlaveInternal MultiprocessMasterSlaveInternal
#endif  // defined(OS_ANDROID)
TEST(IPCSupportTest, MAYBE_MultiprocessMasterSlaveInternal) {
  embedder::SimplePlatformSupport platform_support;
  base::TestIOThread test_io_thread(base::TestIOThread::kAutoStart);
  TestMasterProcessDelegate master_process_delegate;
  // Note: Run process delegate methods on the I/O thread.
  IPCSupport ipc_support(&platform_support, embedder::ProcessType::MASTER,
                         test_io_thread.task_runner(), &master_process_delegate,
                         test_io_thread.task_runner(),
                         embedder::ScopedPlatformHandle());

  ConnectionIdentifier connection_id =
      ipc_support.GenerateConnectionIdentifier();
  mojo::test::MultiprocessTestHelper multiprocess_test_helper;
  ProcessIdentifier slave_id = kInvalidProcessIdentifier;
  embedder::ScopedPlatformHandle second_platform_handle =
      ipc_support.ConnectToSlaveInternal(
          connection_id, nullptr,
          multiprocess_test_helper.server_platform_handle.Pass(), &slave_id);
  ASSERT_TRUE(second_platform_handle.is_valid());
  EXPECT_NE(slave_id, kInvalidProcessIdentifier);
  EXPECT_NE(slave_id, kMasterProcessIdentifier);

  multiprocess_test_helper.StartChildWithExtraSwitch(
      "MultiprocessMasterSlaveInternal", kConnectionIdFlag,
      connection_id.ToString());

  // We write a '?'. The slave should write a '!' in response.
  size_t n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingWrite(second_platform_handle.get(), "?", 1, &n));
  EXPECT_EQ(1u, n);

  char c = '\0';
  n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingRead(second_platform_handle.get(), &c, 1, &n));
  EXPECT_EQ(1u, n);
  EXPECT_EQ('!', c);

  EXPECT_TRUE(master_process_delegate.TryWaitForOnSlaveDisconnect());
  EXPECT_TRUE(multiprocess_test_helper.WaitForChildTestShutdown());

  test_io_thread.PostTaskAndWait(FROM_HERE,
                                 base::Bind(&IPCSupport::ShutdownOnIOThread,
                                            base::Unretained(&ipc_support)));
}

MOJO_MULTIPROCESS_TEST_CHILD_TEST(MultiprocessMasterSlaveInternal) {
  embedder::ScopedPlatformHandle client_platform_handle =
      mojo::test::MultiprocessTestHelper::client_platform_handle.Pass();
  ASSERT_TRUE(client_platform_handle.is_valid());

  embedder::SimplePlatformSupport platform_support;
  base::TestIOThread test_io_thread(base::TestIOThread::kAutoStart);
  TestSlaveProcessDelegate slave_process_delegate;
  // Note: Run process delegate methods on the I/O thread.
  IPCSupport ipc_support(&platform_support, embedder::ProcessType::SLAVE,
                         test_io_thread.task_runner(), &slave_process_delegate,
                         test_io_thread.task_runner(),
                         client_platform_handle.Pass());

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  ASSERT_TRUE(command_line.HasSwitch(kConnectionIdFlag));
  bool ok = false;
  ConnectionIdentifier connection_id = ConnectionIdentifier::FromString(
      command_line.GetSwitchValueASCII(kConnectionIdFlag), &ok);
  ASSERT_TRUE(ok);

  embedder::ScopedPlatformHandle second_platform_handle =
      ipc_support.ConnectToMasterInternal(connection_id);
  ASSERT_TRUE(second_platform_handle.is_valid());

  // The master should write a '?'. We'll write a '!' in response.
  char c = '\0';
  size_t n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingRead(second_platform_handle.get(), &c, 1, &n));
  EXPECT_EQ(1u, n);
  EXPECT_EQ('?', c);

  n = 0;
  EXPECT_TRUE(
      mojo::test::BlockingWrite(second_platform_handle.get(), "!", 1, &n));
  EXPECT_EQ(1u, n);

  test_io_thread.PostTaskAndWait(FROM_HERE,
                                 base::Bind(&IPCSupport::ShutdownOnIOThread,
                                            base::Unretained(&ipc_support)));
}

// TODO(vtl): Also test the case of the master "dying" before the slave. (The
// slave should get OnMasterDisconnect(), which we currently don't test.)

}  // namespace system
}  // namespace mojo
