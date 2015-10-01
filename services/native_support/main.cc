// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/threading/sequenced_worker_pool.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/native_support/public/interfaces/process.mojom.h"
#include "services/native_support/process_impl.h"

namespace native_support {

// TODO(vtl): Having to manually choose an arbitrary number is dumb.
const size_t kMaxWorkerThreads = 16;

class NativeSupportApp : public mojo::ApplicationDelegate,
                         public mojo::InterfaceFactory<Process> {
 public:
  NativeSupportApp() {}
  ~NativeSupportApp() override {
    if (worker_pool_)
      worker_pool_->Shutdown();
  }

 private:
  // |mojo::ApplicationDelegate| override:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<Process>(this);
    return true;
  }

  // |InterfaceFactory<Process>| implementation:
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<Process> request) override {
    if (!worker_pool_) {
      worker_pool_ = new base::SequencedWorkerPool(kMaxWorkerThreads,
                                                   "NativeSupportWorker");
    }
    new ProcessImpl(worker_pool_, connection, request.Pass());
  }

  scoped_refptr<base::SequencedWorkerPool> worker_pool_;

  DISALLOW_COPY_AND_ASSIGN(NativeSupportApp);
};

}  // namespace native_support

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new native_support::NativeSupportApp());
  // We need an I/O message loop, since we'll want to watch FDs.
  runner.set_message_loop_type(base::MessageLoop::TYPE_IO);
  return runner.Run(application_request);
}
