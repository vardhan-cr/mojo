// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/context.h"

#include "base/files/file_path.h"
#include "base/message_loop/message_loop.h"
#include "mojo/common/message_pump_mojo.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace shell {
namespace {

TEST(ContextTest, Paths) {
  Context context;
  base::MessageLoop message_loop(
      scoped_ptr<base::MessagePump>(new common::MessagePumpMojo()));
  context.Init();

  EXPECT_FALSE(context.mojo_shell_path().empty());
  EXPECT_FALSE(context.mojo_shell_child_path().empty());
  EXPECT_TRUE(context.mojo_shell_path().IsAbsolute());
  EXPECT_TRUE(context.mojo_shell_child_path().IsAbsolute());

  context.Shutdown();
}

}  // namespace
}  // namespace shell
}  // namespace mojo
