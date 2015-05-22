// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/channel.h"

#include "base/bind.h"
#include "base/location.h"
#include "mojo/edk/system/channel_endpoint.h"
#include "mojo/edk/system/channel_endpoint_id.h"
#include "mojo/edk/system/channel_test_base.h"
#include "mojo/edk/system/message_pipe.h"
#include "mojo/edk/system/test_utils.h"
#include "mojo/edk/system/waiter.h"

namespace mojo {
namespace system {
namespace {

using ChannelTest = test::ChannelTestBase;

// ChannelTest.InitShutdown ----------------------------------------------------

TEST_F(ChannelTest, InitShutdown) {
  io_thread()->PostTaskAndWait(FROM_HERE,
                               base::Bind(&ChannelTest::CreateChannelOnIOThread,
                                          base::Unretained(this)));
  ASSERT_TRUE(channel());

  io_thread()->PostTaskAndWait(
      FROM_HERE,
      base::Bind(&ChannelTest::InitChannelOnIOThread, base::Unretained(this)));

  io_thread()->PostTaskAndWait(
      FROM_HERE, base::Bind(&ChannelTest::ShutdownChannelOnIOThread,
                            base::Unretained(this)));

  // Okay to destroy |Channel| on not-the-I/O-thread.
  EXPECT_TRUE(channel()->HasOneRef());
  *mutable_channel() = nullptr;
}

// ChannelTest.CloseBeforeAttachAndRun -----------------------------------------

TEST_F(ChannelTest, CloseBeforeRun) {
  io_thread()->PostTaskAndWait(FROM_HERE,
                               base::Bind(&ChannelTest::CreateChannelOnIOThread,
                                          base::Unretained(this)));
  ASSERT_TRUE(channel());

  io_thread()->PostTaskAndWait(
      FROM_HERE,
      base::Bind(&ChannelTest::InitChannelOnIOThread, base::Unretained(this)));

  scoped_refptr<ChannelEndpoint> channel_endpoint;
  scoped_refptr<MessagePipe> mp(
      MessagePipe::CreateLocalProxy(&channel_endpoint));

  mp->Close(0);

  channel()->SetBootstrapEndpoint(channel_endpoint);

  io_thread()->PostTaskAndWait(
      FROM_HERE, base::Bind(&ChannelTest::ShutdownChannelOnIOThread,
                            base::Unretained(this)));

  EXPECT_TRUE(channel()->HasOneRef());
}

// ChannelTest.ShutdownAfterAttachAndRun ---------------------------------------

TEST_F(ChannelTest, ShutdownAfterAttach) {
  io_thread()->PostTaskAndWait(FROM_HERE,
                               base::Bind(&ChannelTest::CreateChannelOnIOThread,
                                          base::Unretained(this)));
  ASSERT_TRUE(channel());

  io_thread()->PostTaskAndWait(
      FROM_HERE,
      base::Bind(&ChannelTest::InitChannelOnIOThread, base::Unretained(this)));

  scoped_refptr<ChannelEndpoint> channel_endpoint;
  scoped_refptr<MessagePipe> mp(
      MessagePipe::CreateLocalProxy(&channel_endpoint));

  channel()->SetBootstrapEndpoint(channel_endpoint);

  Waiter waiter;
  waiter.Init();
  ASSERT_EQ(
      MOJO_RESULT_OK,
      mp->AddAwakable(0, &waiter, MOJO_HANDLE_SIGNAL_READABLE, 123, nullptr));

  // Don't wait for the shutdown to run ...
  io_thread()->PostTask(FROM_HERE,
                        base::Bind(&ChannelTest::ShutdownChannelOnIOThread,
                                   base::Unretained(this)));

  // ... since this |Wait()| should fail once the channel is shut down.
  EXPECT_EQ(MOJO_RESULT_FAILED_PRECONDITION,
            waiter.Wait(MOJO_DEADLINE_INDEFINITE, nullptr));
  HandleSignalsState hss;
  mp->RemoveAwakable(0, &waiter, &hss);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  mp->Close(0);

  EXPECT_TRUE(channel()->HasOneRef());
}

// ChannelTest.WaitAfterAttachRunAndShutdown -----------------------------------

TEST_F(ChannelTest, WaitAfterAttachRunAndShutdown) {
  io_thread()->PostTaskAndWait(FROM_HERE,
                               base::Bind(&ChannelTest::CreateChannelOnIOThread,
                                          base::Unretained(this)));
  ASSERT_TRUE(channel());

  io_thread()->PostTaskAndWait(
      FROM_HERE,
      base::Bind(&ChannelTest::InitChannelOnIOThread, base::Unretained(this)));

  scoped_refptr<ChannelEndpoint> channel_endpoint;
  scoped_refptr<MessagePipe> mp(
      MessagePipe::CreateLocalProxy(&channel_endpoint));

  channel()->SetBootstrapEndpoint(channel_endpoint);

  io_thread()->PostTaskAndWait(
      FROM_HERE, base::Bind(&ChannelTest::ShutdownChannelOnIOThread,
                            base::Unretained(this)));

  Waiter waiter;
  waiter.Init();
  HandleSignalsState hss;
  EXPECT_EQ(
      MOJO_RESULT_FAILED_PRECONDITION,
      mp->AddAwakable(0, &waiter, MOJO_HANDLE_SIGNAL_READABLE, 123, &hss));
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfied_signals);
  EXPECT_EQ(MOJO_HANDLE_SIGNAL_PEER_CLOSED, hss.satisfiable_signals);

  mp->Close(0);

  EXPECT_TRUE(channel()->HasOneRef());
}

// TODO(vtl): More. ------------------------------------------------------------

}  // namespace
}  // namespace system
}  // namespace mojo
