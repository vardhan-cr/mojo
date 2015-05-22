// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/channel_test_base.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/system/raw_channel.h"

namespace mojo {
namespace system {
namespace test {

ChannelTestBase::ChannelTestBase()
    : io_thread_(base::TestIOThread::kAutoStart) {
}

ChannelTestBase::~ChannelTestBase() {
}

void ChannelTestBase::SetUp() {
  io_thread_.PostTaskAndWait(
      FROM_HERE,
      base::Bind(&ChannelTestBase::SetUpOnIOThread, base::Unretained(this)));
}

void ChannelTestBase::CreateChannelOnIOThread() {
  CHECK_EQ(base::MessageLoop::current(), io_thread()->message_loop());
  channel_ = new Channel(&platform_support_);
}

void ChannelTestBase::InitChannelOnIOThread() {
  CHECK_EQ(base::MessageLoop::current(), io_thread()->message_loop());

  CHECK(raw_channel_);
  CHECK(channel_);
  channel_->Init(raw_channel_.Pass());
}

void ChannelTestBase::ShutdownChannelOnIOThread() {
  CHECK_EQ(base::MessageLoop::current(), io_thread()->message_loop());

  CHECK(channel_);
  channel_->Shutdown();
}

void ChannelTestBase::SetUpOnIOThread() {
  CHECK_EQ(base::MessageLoop::current(), io_thread()->message_loop());

  embedder::PlatformChannelPair channel_pair;
  raw_channel_ = RawChannel::Create(channel_pair.PassServerHandle()).Pass();
  other_platform_handle_ = channel_pair.PassClientHandle();
}

}  // namespace test
}  // namespace system
}  // namespace mojo
