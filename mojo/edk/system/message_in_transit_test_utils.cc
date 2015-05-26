// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/message_in_transit_test_utils.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace system {
namespace test {

scoped_ptr<MessageInTransit> MakeTestMessage(unsigned id) {
  return make_scoped_ptr(
      new MessageInTransit(MessageInTransit::kTypeEndpointClient,
                           MessageInTransit::kSubtypeEndpointClientData,
                           static_cast<uint32_t>(sizeof(id)), &id));
}

void VerifyTestMessage(MessageInTransit* message, unsigned id) {
  ASSERT_TRUE(message);
  EXPECT_EQ(MessageInTransit::kTypeEndpointClient, message->type());
  EXPECT_EQ(MessageInTransit::kSubtypeEndpointClientData, message->subtype());
  EXPECT_EQ(sizeof(id), message->num_bytes());
  EXPECT_EQ(id, *static_cast<const unsigned*>(message->bytes()));
}

}  // namespace test
}  // namespace system
}  // namespace mojo
