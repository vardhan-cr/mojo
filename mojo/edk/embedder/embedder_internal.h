// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This header contains internal details for the *implementation* of the
// embedder API. It should not be included by any public header (nor by users of
// the embedder API).

#ifndef MOJO_EDK_EMBEDDER_EMBEDDER_INTERNAL_H_
#define MOJO_EDK_EMBEDDER_EMBEDDER_INTERNAL_H_

#include <stdint.h>

#include "mojo/edk/embedder/process_type.h"

namespace base {
class TaskRunner;
}

namespace mojo {

namespace system {

class ChannelManager;
class Core;

// Repeat a typedef in mojo/edk/system/channel_manager.h, to avoid including it.
typedef uint64_t ChannelId;

}  // namespace system

namespace embedder {

class PlatformSupport;
class ProcessDelegate;

// This is a type that's opaque to users of the embedder API (which only
// gives/takes |ChannelInfo*|s). We make it a struct to make it
// template-friendly.
struct ChannelInfo {
  explicit ChannelInfo(system::ChannelId channel_id = 0)
      : channel_id(channel_id) {}

  system::ChannelId channel_id;
};

namespace internal {

// Instance of |PlatformSupport| to use.
extern PlatformSupport* g_platform_support;

// Instance of |Core| used by the system functions (|Mojo...()|).
extern system::Core* g_core;

extern ProcessType g_process_type;

// Note: This needs to be |AddRef()|ed/|Release()|d.
extern base::TaskRunner* g_delegate_thread_task_runner;

extern ProcessDelegate* g_process_delegate;

// Note: This needs to be |AddRef()|ed/|Release()|d.
extern base::TaskRunner* g_io_thread_task_runner;

// Instance of |ChannelManager| used by the channel management functions
// (|mojo::embedder::CreateChannel()|, etc.).
extern system::ChannelManager* g_channel_manager;

}  // namespace internal

}  // namepace embedder

}  // namespace mojo

#endif  // MOJO_EDK_EMBEDDER_EMBEDDER_INTERNAL_H_
