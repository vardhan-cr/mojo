// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/message_in_transit_queue.h"

#include "mojo/edk/system/message_in_transit_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace system {
namespace {

TEST(MessageInTransitQueueTest, Basic) {
  MessageInTransitQueue queue;
  EXPECT_TRUE(queue.IsEmpty());

  queue.AddMessage(test::MakeTestMessage(1));
  ASSERT_FALSE(queue.IsEmpty());

  test::VerifyTestMessage(queue.PeekMessage(), 1);
  ASSERT_FALSE(queue.IsEmpty());

  queue.AddMessage(test::MakeTestMessage(2));
  queue.AddMessage(test::MakeTestMessage(3));
  ASSERT_FALSE(queue.IsEmpty());

  test::VerifyTestMessage(queue.GetMessage().get(), 1);
  ASSERT_FALSE(queue.IsEmpty());

  test::VerifyTestMessage(queue.PeekMessage(), 2);
  ASSERT_FALSE(queue.IsEmpty());

  queue.DiscardMessage();
  ASSERT_FALSE(queue.IsEmpty());

  test::VerifyTestMessage(queue.GetMessage().get(), 3);
  EXPECT_TRUE(queue.IsEmpty());

  queue.AddMessage(test::MakeTestMessage(4));
  ASSERT_FALSE(queue.IsEmpty());

  test::VerifyTestMessage(queue.PeekMessage(), 4);
  ASSERT_FALSE(queue.IsEmpty());

  queue.Clear();
  EXPECT_TRUE(queue.IsEmpty());
}

TEST(MessageInTransitQueueTest, Swap) {
  MessageInTransitQueue queue1;
  MessageInTransitQueue queue2;

  queue1.AddMessage(test::MakeTestMessage(1));
  queue1.AddMessage(test::MakeTestMessage(2));
  queue1.AddMessage(test::MakeTestMessage(3));

  queue2.AddMessage(test::MakeTestMessage(4));
  queue2.AddMessage(test::MakeTestMessage(5));

  queue1.Swap(&queue2);
  test::VerifyTestMessage(queue1.GetMessage().get(), 4);
  test::VerifyTestMessage(queue1.GetMessage().get(), 5);
  EXPECT_TRUE(queue1.IsEmpty());

  queue1.Swap(&queue2);
  EXPECT_TRUE(queue2.IsEmpty());

  test::VerifyTestMessage(queue1.GetMessage().get(), 1);
  test::VerifyTestMessage(queue1.GetMessage().get(), 2);
  test::VerifyTestMessage(queue1.GetMessage().get(), 3);
  EXPECT_TRUE(queue1.IsEmpty());
}

}  // namespace
}  // namespace system
}  // namespace mojo
