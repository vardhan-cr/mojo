// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include "base/at_exit.h"
#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/thread_task_runner_handle.h"
#include "base/threading/thread.h"
#include "base/threading/thread_checker.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/edk/embedder/slave_process_delegate.h"
#include "mojo/message_pump/message_pump_mojo.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/system/core.h"
#include "shell/child_controller.mojom.h"
#include "shell/child_switches.h"
#include "shell/init.h"
#include "shell/native_application_support.h"

namespace shell {
namespace {

// Blocker ---------------------------------------------------------------------

// Blocks a thread until another thread unblocks it, at which point it unblocks
// and runs a closure provided by that thread.
class Blocker {
 public:
  class Unblocker {
   public:
    explicit Unblocker(Blocker* blocker = nullptr) : blocker_(blocker) {}
    ~Unblocker() {}

    void Unblock(base::Closure run_after) {
      DCHECK(blocker_);
      DCHECK(blocker_->run_after_.is_null());
      blocker_->run_after_ = run_after;
      blocker_->event_.Signal();
      blocker_ = nullptr;
    }

   private:
    Blocker* blocker_;

    // Copy and assign allowed.
  };

  Blocker() : event_(true, false) {}
  ~Blocker() {}

  void Block() {
    DCHECK(run_after_.is_null());
    event_.Wait();
    if (!run_after_.is_null())
      run_after_.Run();
  }

  Unblocker GetUnblocker() { return Unblocker(this); }

 private:
  base::WaitableEvent event_;
  base::Closure run_after_;

  DISALLOW_COPY_AND_ASSIGN(Blocker);
};

// AppContext ------------------------------------------------------------------

class ChildControllerImpl;

// Should be created and initialized on the main thread.
class AppContext : public mojo::embedder::SlaveProcessDelegate {
 public:
  AppContext()
      : io_thread_("io_thread"), controller_thread_("controller_thread") {}
  ~AppContext() override {}

  void Init(mojo::embedder::ScopedPlatformHandle platform_handle) {
    // Initialize Mojo before starting any threads.
    mojo::embedder::Init(
        make_scoped_ptr(new mojo::embedder::SimplePlatformSupport()));

    // Create and start our I/O thread.
    base::Thread::Options io_thread_options(base::MessageLoop::TYPE_IO, 0);
    CHECK(io_thread_.StartWithOptions(io_thread_options));
    io_runner_ = io_thread_.message_loop_proxy().get();
    CHECK(io_runner_.get());

    // Create and start our controller thread.
    base::Thread::Options controller_thread_options;
    controller_thread_options.message_loop_type =
        base::MessageLoop::TYPE_CUSTOM;
    controller_thread_options.message_pump_factory =
        base::Bind(&mojo::common::MessagePumpMojo::Create);
    CHECK(controller_thread_.StartWithOptions(controller_thread_options));
    controller_runner_ = controller_thread_.message_loop_proxy().get();
    CHECK(controller_runner_.get());

    mojo::embedder::InitIPCSupport(mojo::embedder::ProcessType::SLAVE,
                                   controller_runner_, this, io_runner_,
                                   platform_handle.Pass());
  }

  void Shutdown() {
    Blocker blocker;
    shutdown_unblocker_ = blocker.GetUnblocker();
    controller_runner_->PostTask(
        FROM_HERE, base::Bind(&AppContext::ShutdownOnControllerThread,
                              base::Unretained(this)));
    blocker.Block();
  }

  base::SingleThreadTaskRunner* io_runner() const { return io_runner_.get(); }

  base::SingleThreadTaskRunner* controller_runner() const {
    return controller_runner_.get();
  }

  ChildControllerImpl* controller() const { return controller_.get(); }

  void set_controller(scoped_ptr<ChildControllerImpl> controller) {
    controller_ = controller.Pass();
  }

 private:
  void ShutdownOnControllerThread() {
    // First, destroy the controller.
    controller_.reset();

    // Next shutdown IPC. We'll unblock the main thread in OnShutdownComplete().
    mojo::embedder::ShutdownIPCSupport();
  }

  // SlaveProcessDelegate implementation.
  void OnShutdownComplete() override {
    shutdown_unblocker_.Unblock(base::Closure());
  }

  void OnMasterDisconnect() override {
    // We've lost the connection to the master process. This is not recoverable.
    LOG(ERROR) << "Disconnected from master";
    _exit(1);
  }

  base::Thread io_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> io_runner_;

  base::Thread controller_thread_;
  scoped_refptr<base::SingleThreadTaskRunner> controller_runner_;

  // Accessed only on the controller thread.
  scoped_ptr<ChildControllerImpl> controller_;

  // Used to unblock the main thread on shutdown.
  Blocker::Unblocker shutdown_unblocker_;

  DISALLOW_COPY_AND_ASSIGN(AppContext);
};

// ChildControllerImpl ---------------------------------------------------------

class ChildControllerImpl : public ChildController {
 public:
  ~ChildControllerImpl() override {
    DCHECK(thread_checker_.CalledOnValidThread());

    // TODO(vtl): Pass in the result from |MainMain()|.
    on_app_complete_.Run(MOJO_RESULT_UNIMPLEMENTED);
  }

  // To be executed on the controller thread. Creates the |ChildController|,
  // etc.
  static void Init(AppContext* app_context,
                   const std::string& child_connection_id,
                   const Blocker::Unblocker& unblocker) {
    DCHECK(app_context);
    DCHECK(!app_context->controller());

    scoped_ptr<ChildControllerImpl> impl(
        new ChildControllerImpl(app_context, unblocker));
    mojo::ScopedMessagePipeHandle host_message_pipe(
        mojo::embedder::ConnectToMaster(
            child_connection_id,
            base::Bind(&ChildControllerImpl::DidConnectToMaster,
                       base::Unretained(impl.get())),
            base::ThreadTaskRunnerHandle::Get(), &impl->channel_info_));
    DCHECK(impl->channel_info_);
    impl->Bind(host_message_pipe.Pass());

    app_context->set_controller(impl.Pass());
  }

  void Bind(mojo::ScopedMessagePipeHandle handle) {
    binding_.Bind(handle.Pass());
  }

  // |ChildController| methods:
  void StartApp(const mojo::String& app_path,
                mojo::InterfaceRequest<mojo::Application> application_request,
                const StartAppCallback& on_app_complete) override {
    DVLOG(2) << "ChildControllerImpl::StartApp(" << app_path << ", ...)";
    DCHECK(thread_checker_.CalledOnValidThread());

    on_app_complete_ = on_app_complete;
    unblocker_.Unblock(base::Bind(&ChildControllerImpl::StartAppOnMainThread,
                                  base::FilePath::FromUTF8Unsafe(app_path),
                                  base::Passed(&application_request)));
  }

  void ExitNow(int32_t exit_code) override {
    DVLOG(2) << "ChildControllerImpl::ExitNow(" << exit_code << ")";
    _exit(exit_code);
  }

 private:
  ChildControllerImpl(AppContext* app_context,
                      const Blocker::Unblocker& unblocker)
      : app_context_(app_context),
        unblocker_(unblocker),
        channel_info_(nullptr),
        binding_(this) {
    binding_.set_connection_error_handler([this]() { OnConnectionError(); });
  }

  void OnConnectionError() {
    // A connection error means the connection to the shell is lost. This is not
    // recoverable.
    LOG(ERROR) << "Connection error to the shell";
    _exit(1);
  }

  // Callback for |mojo::embedder::ConnectToMaster()|.
  void DidConnectToMaster() {
    DVLOG(2) << "ChildControllerImpl::DidCreateChannel()";
    DCHECK(thread_checker_.CalledOnValidThread());
  }

  static void StartAppOnMainThread(
      const base::FilePath& app_path,
      mojo::InterfaceRequest<mojo::Application> application_request) {
    // TODO(vtl): This is copied from in_process_native_runner.cc.
    DVLOG(2) << "Loading/running Mojo app from " << app_path.value()
             << " out of process";

    // We intentionally don't unload the native library as its lifetime is the
    // same as that of the process.
    base::NativeLibrary app_library = LoadNativeApplication(app_path);
    RunNativeApplication(app_library, application_request.Pass());
  }

  base::ThreadChecker thread_checker_;
  AppContext* const app_context_;
  Blocker::Unblocker unblocker_;
  StartAppCallback on_app_complete_;

  mojo::embedder::ChannelInfo* channel_info_;
  mojo::Binding<ChildController> binding_;

  DISALLOW_COPY_AND_ASSIGN(ChildControllerImpl);
};

}  // namespace
}  // namespace shell

int main(int argc, char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  shell::InitializeLogging();

  // Make sure that we're really meant to be invoked as the child process.

  CHECK(command_line.HasSwitch(switches::kChildConnectionId));
  std::string child_connection_id =
      command_line.GetSwitchValueASCII(switches::kChildConnectionId);
  CHECK(!child_connection_id.empty());

  mojo::embedder::ScopedPlatformHandle platform_handle =
      mojo::embedder::PlatformChannelPair::PassClientHandleFromParentProcess(
          command_line);
  CHECK(platform_handle.is_valid());

  shell::AppContext app_context;
  app_context.Init(platform_handle.Pass());

  shell::Blocker blocker;
  app_context.controller_runner()->PostTask(
      FROM_HERE, base::Bind(&shell::ChildControllerImpl::Init,
                            base::Unretained(&app_context), child_connection_id,
                            blocker.GetUnblocker()));
  // This will block, then run whatever the controller wants.
  blocker.Block();

  app_context.Shutdown();

  return 0;
}
