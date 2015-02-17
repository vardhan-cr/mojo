// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/embedder/embedder.h"

#include "base/atomicops.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop_proxy.h"
#include "base/task_runner.h"
#include "mojo/edk/embedder/embedder_internal.h"
#include "mojo/edk/embedder/master_process_delegate.h"
#include "mojo/edk/embedder/platform_support.h"
#include "mojo/edk/embedder/process_delegate.h"
#include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/edk/system/channel.h"
#include "mojo/edk/system/channel_manager.h"
#include "mojo/edk/system/configuration.h"
#include "mojo/edk/system/core.h"
#include "mojo/edk/system/message_pipe_dispatcher.h"
#include "mojo/edk/system/platform_handle_dispatcher.h"
#include "mojo/edk/system/raw_channel.h"

namespace mojo {
namespace embedder {

namespace {

// TODO(vtl): For now, we need this to be thread-safe (since theoretically we
// currently support multiple channel creation threads -- possibly one per
// channel). Eventually, we won't need it to be thread-safe (we'll require a
// single I/O thread), and eventually we won't need it at all. Remember to
// remove the base/atomicops.h include.
system::ChannelId MakeChannelId() {
  // Note that |AtomicWord| is signed.
  static base::subtle::AtomicWord counter = 0;

  base::subtle::AtomicWord new_counter_value =
      base::subtle::NoBarrier_AtomicIncrement(&counter, 1);
  // Don't allow the counter to wrap. Note that any (strictly) positive value is
  // a valid |ChannelId| (and |NoBarrier_AtomicIncrement()| returns the value
  // post-increment).
  CHECK_GT(new_counter_value, 0);
  // Use "negative" values for these IDs, so that we'll also be able to use
  // "positive" "process identifiers" (see connection_manager.h) as IDs (and
  // they won't conflict).
  return static_cast<system::ChannelId>(-new_counter_value);
}

}  // namespace

namespace internal {

// Declared in embedder_internal.h.
PlatformSupport* g_platform_support = nullptr;
system::Core* g_core = nullptr;
ProcessType g_process_type = ProcessType::UNINITIALIZED;
system::ChannelManager* g_channel_manager = nullptr;
base::TaskRunner* g_delegate_thread_task_runner = nullptr;
ProcessDelegate* g_process_delegate = nullptr;

}  // namespace internal

Configuration* GetConfiguration() {
  return system::GetMutableConfiguration();
}

void Init(scoped_ptr<PlatformSupport> platform_support) {
  DCHECK(platform_support);

  DCHECK(!internal::g_platform_support);
  internal::g_platform_support = platform_support.release();

  DCHECK(!internal::g_core);
  internal::g_core = new system::Core(internal::g_platform_support);

  DCHECK(!internal::g_channel_manager);
  internal::g_channel_manager =
      new system::ChannelManager(internal::g_platform_support);
}

MojoResult AsyncWait(MojoHandle handle,
                     MojoHandleSignals signals,
                     const base::Callback<void(MojoResult)>& callback) {
  return internal::g_core->AsyncWait(handle, signals, callback);
}

MojoResult CreatePlatformHandleWrapper(
    ScopedPlatformHandle platform_handle,
    MojoHandle* platform_handle_wrapper_handle) {
  DCHECK(platform_handle_wrapper_handle);

  scoped_refptr<system::Dispatcher> dispatcher(
      new system::PlatformHandleDispatcher(platform_handle.Pass()));

  DCHECK(internal::g_core);
  MojoHandle h = internal::g_core->AddDispatcher(dispatcher);
  if (h == MOJO_HANDLE_INVALID) {
    LOG(ERROR) << "Handle table full";
    dispatcher->Close();
    return MOJO_RESULT_RESOURCE_EXHAUSTED;
  }

  *platform_handle_wrapper_handle = h;
  return MOJO_RESULT_OK;
}

MojoResult PassWrappedPlatformHandle(MojoHandle platform_handle_wrapper_handle,
                                     ScopedPlatformHandle* platform_handle) {
  DCHECK(platform_handle);

  DCHECK(internal::g_core);
  scoped_refptr<system::Dispatcher> dispatcher(
      internal::g_core->GetDispatcher(platform_handle_wrapper_handle));
  if (!dispatcher)
    return MOJO_RESULT_INVALID_ARGUMENT;

  if (dispatcher->GetType() != system::Dispatcher::kTypePlatformHandle)
    return MOJO_RESULT_INVALID_ARGUMENT;

  *platform_handle =
      static_cast<system::PlatformHandleDispatcher*>(dispatcher.get())
          ->PassPlatformHandle()
          .Pass();
  return MOJO_RESULT_OK;
}

void InitIPCSupport(ProcessType process_type,
                    scoped_refptr<base::TaskRunner> delegate_thread_task_runner,
                    ProcessDelegate* process_delegate,
                    scoped_refptr<base::TaskRunner> io_thread_task_runner,
                    ScopedPlatformHandle platform_handle) {
  // |Init()| must have already been called.
  DCHECK(internal::g_core);
  // And not |InitIPCSupport()| (without |ShutdownIPCSupport()|).
  DCHECK(internal::g_process_type == ProcessType::UNINITIALIZED);

  internal::g_process_type = process_type;

  DCHECK(delegate_thread_task_runner);
  internal::g_delegate_thread_task_runner = delegate_thread_task_runner.get();
  internal::g_delegate_thread_task_runner->AddRef();

  DCHECK(process_delegate->GetType() == process_type);
  internal::g_process_delegate = process_delegate;

  switch (process_type) {
    case ProcessType::UNINITIALIZED:
      CHECK(false);
      break;
    case ProcessType::NONE:
      // TODO(vtl): Construct a "DummyConnectionManager" here, or something.
      break;
    case ProcessType::MASTER:
      // TODO(vtl): Construct a |MasterConnectionManager| here.
      break;
    case ProcessType::SLAVE:
      // TODO(vtl): Construct a |SlaveConnectionManager| here.
      break;
  }
}

void ShutdownIPCSupportOnIOThread() {
  DCHECK(internal::g_process_type != ProcessType::UNINITIALIZED);

  // TODO(vtl): Tear down the connection manager here, once it's been created.

  internal::g_delegate_thread_task_runner->Release();
  internal::g_delegate_thread_task_runner = nullptr;

  internal::g_process_delegate = nullptr;

  internal::g_process_type = ProcessType::UNINITIALIZED;
}

void ShutdownIPCSupport() {
  DCHECK(internal::g_process_type != ProcessType::UNINITIALIZED);

  // TODO(vtl): This will actually have to tear down other stuff. For now, just
  // post the shutdown complete call.

  scoped_refptr<base::TaskRunner> delegate_thread_task_runner(
      internal::g_delegate_thread_task_runner);
  internal::g_delegate_thread_task_runner->Release();
  internal::g_delegate_thread_task_runner = nullptr;

  ProcessDelegate* process_delegate = internal::g_process_delegate;
  internal::g_process_delegate = nullptr;

  internal::g_process_type = ProcessType::UNINITIALIZED;

  bool ok = delegate_thread_task_runner->PostTask(
      FROM_HERE, base::Bind(&ProcessDelegate::OnShutdownComplete,
                            base::Unretained(process_delegate)));
  DCHECK(ok);
}

// TODO(vtl): Write tests for this.
ScopedMessagePipeHandle CreateChannelOnIOThread(
    ScopedPlatformHandle platform_handle,
    ChannelInfo** channel_info) {
  DCHECK(platform_handle.is_valid());
  DCHECK(channel_info);

  *channel_info = new ChannelInfo(MakeChannelId());
  scoped_refptr<system::MessagePipeDispatcher> dispatcher =
      internal::g_channel_manager->CreateChannelOnIOThread(
          (*channel_info)->channel_id, platform_handle.Pass());

  ScopedMessagePipeHandle rv(
      MessagePipeHandle(internal::g_core->AddDispatcher(dispatcher)));
  CHECK(rv.is_valid());
  // TODO(vtl): The |.Pass()| below is only needed due to an MSVS bug; remove it
  // once that's fixed.
  return rv.Pass();
}

ScopedMessagePipeHandle CreateChannel(
    ScopedPlatformHandle platform_handle,
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    const DidCreateChannelCallback& callback,
    scoped_refptr<base::TaskRunner> callback_thread_task_runner) {
  DCHECK(platform_handle.is_valid());
  DCHECK(io_thread_task_runner);
  DCHECK(!callback.is_null());

  system::ChannelId channel_id = MakeChannelId();
  scoped_ptr<ChannelInfo> channel_info(new ChannelInfo(channel_id));
  scoped_refptr<system::MessagePipeDispatcher> dispatcher =
      internal::g_channel_manager->CreateChannel(
          channel_id, platform_handle.Pass(), io_thread_task_runner,
          base::Bind(callback, base::Unretained(channel_info.release())),
          callback_thread_task_runner);

  ScopedMessagePipeHandle rv(
      MessagePipeHandle(internal::g_core->AddDispatcher(dispatcher)));
  CHECK(rv.is_valid());
  // TODO(vtl): The |.Pass()| below is only needed due to an MSVS bug; remove it
  // once that's fixed.
  return rv.Pass();
}

// TODO(vtl): Write tests for this.
void DestroyChannelOnIOThread(ChannelInfo* channel_info) {
  DCHECK(channel_info);
  DCHECK(channel_info->channel_id);
  DCHECK(internal::g_channel_manager);
  internal::g_channel_manager->ShutdownChannelOnIOThread(
      channel_info->channel_id);
  delete channel_info;
}

// TODO(vtl): Write tests for this.
void DestroyChannel(
    ChannelInfo* channel_info,
    const DidDestroyChannelCallback& callback,
    scoped_refptr<base::TaskRunner> callback_thread_task_runner) {
  DCHECK(channel_info);
  DCHECK(channel_info->channel_id);
  DCHECK(!callback.is_null());
  DCHECK(internal::g_channel_manager);
  internal::g_channel_manager->ShutdownChannel(
      channel_info->channel_id, callback, callback_thread_task_runner);
  delete channel_info;
}

void WillDestroyChannelSoon(ChannelInfo* channel_info) {
  DCHECK(channel_info);
  DCHECK(internal::g_channel_manager);
  internal::g_channel_manager->WillShutdownChannel(channel_info->channel_id);
}

}  // namespace embedder
}  // namespace mojo
