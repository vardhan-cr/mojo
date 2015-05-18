// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/test/scoped_ipc_support.h"

#include "base/message_loop/message_loop.h"
#include "mojo/edk/embedder/embedder.h"

namespace mojo {
namespace test {

namespace internal {

ScopedIPCSupportBase::ScopedIPCSupportBase(
    embedder::ProcessType process_type,
    embedder::ProcessDelegate* process_delegate,
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    embedder::ScopedPlatformHandle platform_handle)
    : io_thread_task_runner_(io_thread_task_runner.Pass()),
      event_(true, false) {  // Manual reset.
  // Note: Run delegate methods on the I/O thread.
  embedder::InitIPCSupport(process_type, io_thread_task_runner_,
                           process_delegate, io_thread_task_runner_,
                           platform_handle.Pass());
}

ScopedIPCSupportBase::~ScopedIPCSupportBase() {
  if (base::MessageLoop::current() &&
      base::MessageLoop::current()->task_runner() == io_thread_task_runner_) {
    embedder::ShutdownIPCSupportOnIOThread();
  } else {
    embedder::ShutdownIPCSupport();
    event_.Wait();
  }
}

void ScopedIPCSupportBase::OnShutdownCompleteImpl() {
  event_.Signal();
}

}  // namespace internal

ScopedIPCSupport::ScopedIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner)
    : ScopedIPCSupportBase(embedder::ProcessType::NONE,
                           this,
                           io_thread_task_runner.Pass(),
                           embedder::ScopedPlatformHandle()) {
}

ScopedIPCSupport::~ScopedIPCSupport() {
}

void ScopedIPCSupport::OnShutdownComplete() {
  OnShutdownCompleteImpl();
}

ScopedMasterIPCSupport::ScopedMasterIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner)
    : ScopedIPCSupportBase(embedder::ProcessType::MASTER,
                           this,
                           io_thread_task_runner.Pass(),
                           embedder::ScopedPlatformHandle()) {
}

ScopedMasterIPCSupport::ScopedMasterIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    base::Callback<void(embedder::SlaveInfo slave_info)> on_slave_disconnect)
    : ScopedIPCSupportBase(embedder::ProcessType::MASTER,
                           this,
                           io_thread_task_runner.Pass(),
                           embedder::ScopedPlatformHandle()),
      on_slave_disconnect_(on_slave_disconnect) {
}

ScopedMasterIPCSupport::~ScopedMasterIPCSupport() {
}

void ScopedMasterIPCSupport::OnShutdownComplete() {
  OnShutdownCompleteImpl();
}

void ScopedMasterIPCSupport::OnSlaveDisconnect(embedder::SlaveInfo slave_info) {
  if (!on_slave_disconnect_.is_null())
    on_slave_disconnect_.Run(slave_info);
}

ScopedSlaveIPCSupport::ScopedSlaveIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    embedder::ScopedPlatformHandle platform_handle)
    : ScopedIPCSupportBase(embedder::ProcessType::SLAVE,
                           this,
                           io_thread_task_runner.Pass(),
                           platform_handle.Pass()) {
}

ScopedSlaveIPCSupport::ScopedSlaveIPCSupport(
    scoped_refptr<base::TaskRunner> io_thread_task_runner,
    embedder::ScopedPlatformHandle platform_handle,
    base::Closure on_master_disconnect)
    : ScopedIPCSupportBase(embedder::ProcessType::SLAVE,
                           this,
                           io_thread_task_runner.Pass(),
                           platform_handle.Pass()),
      on_master_disconnect_(on_master_disconnect) {
}

ScopedSlaveIPCSupport::~ScopedSlaveIPCSupport() {
}

void ScopedSlaveIPCSupport::OnShutdownComplete() {
  OnShutdownCompleteImpl();
}

void ScopedSlaveIPCSupport::OnMasterDisconnect() {
  if (!on_master_disconnect_.is_null())
    on_master_disconnect_.Run();
}

}  // namespace test
}  // namespace mojo
