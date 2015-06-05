// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_IPC_SUPPORT_H_
#define MOJO_EDK_SYSTEM_IPC_SUPPORT_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/task_runner.h"
#include "mojo/edk/embedder/process_type.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/embedder/slave_info.h"
#include "mojo/edk/system/connection_identifier.h"
#include "mojo/edk/system/system_impl_export.h"

namespace mojo {

namespace embedder {
class PlatformSupport;
class ProcessDelegate;
}

namespace system {

class ChannelManager;
class ConnectionManager;

// |IPCSupport| encapsulates all the objects that are needed to support IPC for
// a single "process" (whether that be a master or a slave).
//
// ("Process" typically means a real process, but for testing purposes, multiple
// instances can coexist within a single real process.)
//
// Each "process" must have an |embedder::PlatformSupport| and a suitable
// |embedder::ProcessDelegate|, together with an I/O thread and a thread on
// which to call delegate methods (which may be the same as the I/O thread).
//
// For testing purposes within a single real process, except for the I/O thread,
// these may be shared between "processes" (i.e., instances of |IPCSupport|) --
// there must be a separate I/O thread for each |IPCSupport|.
//
// Except for |ShutdownOnIOThread()|, this class is thread-safe. (No methods may
// be called during/after |ShutdownOnIOThread()|.)
class MOJO_SYSTEM_IMPL_EXPORT IPCSupport {
 public:
  // Constructor: initializes for the given |process_type|; |process_delegate|
  // must match the process type. |platform_handle| is only used for slave
  // processes.
  //
  // All the (pointer) arguments must remain alive (and, in the case of task
  // runners, continue to process tasks) until |ShutdownOnIOThread()| has been
  // called.
  IPCSupport(embedder::PlatformSupport* platform_support,
             embedder::ProcessType process_type,
             scoped_refptr<base::TaskRunner> delegate_thread_task_runner,
             embedder::ProcessDelegate* process_delegate,
             scoped_refptr<base::TaskRunner> io_thread_task_runner,
             embedder::ScopedPlatformHandle platform_handle);
  // Note: This object must be shut down before destruction (see
  // |ShutdownOnIOThread()|).
  ~IPCSupport();

  // This must be called (exactly once) on the I/O thread before this object is
  // destroyed (which may happen on any thread).
  void ShutdownOnIOThread();

  // Generates a new (unique) connection identifier, for use with
  // |ConnectToSlave()| and |ConnectToMaster()|, below.
  ConnectionIdentifier GenerateConnectionIdentifier();

  // Called in the master process to connect a slave process to the IPC system.
  //
  // |connection_id| should be a unique connection identifier, which will also
  // be given to the slave (in |ConnectToMaster()|, below). |platform_handle|
  // should be the master's handle to an OS "pipe" between master and slave.
  // This will create a second OS "pipe" between the master and slave and return
  // the master's handle.
  //
  // TODO(vtl): This isn't the right API. It should set up a channel and an
  // initial message pipe.
  embedder::ScopedPlatformHandle ConnectToSlave(
      const ConnectionIdentifier& connection_id,
      embedder::SlaveInfo slave_info,
      embedder::ScopedPlatformHandle platform_handle);

  // Called in a slave process to connect it to the master process and thus the
  // IPC system. See |ConnectToSlave()|, above.
  //
  // TODO(vtl): This isn't the right API. It should set up a channel and an
  // initial message pipe.
  embedder::ScopedPlatformHandle ConnectToMaster(
      const ConnectionIdentifier& connection_id);

  embedder::ProcessType process_type() const { return process_type_; }
  embedder::ProcessDelegate* process_delegate() const {
    return process_delegate_;
  }
  base::TaskRunner* delegate_thread_task_runner() const {
    return delegate_thread_task_runner_.get();
  }
  base::TaskRunner* io_thread_task_runner() const {
    return io_thread_task_runner_.get();
  }
  // TODO(vtl): The things that use the following should probably be moved into
  // this class.
  ChannelManager* channel_manager() const { return channel_manager_.get(); }

 private:
  ConnectionManager* connection_manager() const {
    return connection_manager_.get();
  }

  // These are all set on construction and reset by |ShutdownOnIOThread()|.
  embedder::ProcessType process_type_;
  scoped_refptr<base::TaskRunner> delegate_thread_task_runner_;
  embedder::ProcessDelegate* process_delegate_;
  scoped_refptr<base::TaskRunner> io_thread_task_runner_;

  scoped_ptr<ConnectionManager> connection_manager_;
  scoped_ptr<ChannelManager> channel_manager_;

  DISALLOW_COPY_AND_ASSIGN(IPCSupport);
};

}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_IPC_SUPPORT_H_
