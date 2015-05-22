// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_CHANNEL_TEST_BASE_H_
#define MOJO_EDK_SYSTEM_CHANNEL_TEST_BASE_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/test/test_io_thread.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/edk/system/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace system {

class RawChannel;

namespace test {

// A base class for tests that need a |Channel| set up in a simple way.
class ChannelTestBase : public testing::Test {
 public:
  ChannelTestBase();
  ~ChannelTestBase() override;

  void SetUp() override;

  void CreateChannelOnIOThread();
  void InitChannelOnIOThread();
  void ShutdownChannelOnIOThread();

  base::TestIOThread* io_thread() { return &io_thread_; }
  Channel* channel() { return channel_.get(); }
  scoped_refptr<Channel>* mutable_channel() { return &channel_; }

 private:
  void SetUpOnIOThread();

  embedder::SimplePlatformSupport platform_support_;
  base::TestIOThread io_thread_;
  scoped_ptr<RawChannel> raw_channel_;
  embedder::ScopedPlatformHandle other_platform_handle_;
  scoped_refptr<Channel> channel_;

  DISALLOW_COPY_AND_ASSIGN(ChannelTestBase);
};

}  // namespace test
}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_CHANNEL_TEST_BASE_H_
