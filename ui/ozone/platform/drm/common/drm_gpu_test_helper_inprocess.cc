// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/public/ozone_gpu_test_helper.h"

#include "base/bind.h"
#include "base/threading/thread.h"
#include "base/thread_task_runner_handle.h"
// #include "ipc/ipc_listener.h"
// #include "ipc/ipc_message.h"
// #include "ipc/ipc_sender.h"
// #include "ipc/message_filter.h"
#include "ui/ozone/public/gpu_platform_support.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host_inprocess.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support_inprocess.h"

namespace ui {

namespace {

const int kGpuProcessHostId = 1;

}  // namespace

static void DispatchToGpuPlatformSupportHostTask(Message* msg) {
  auto support = static_cast<DrmGpuPlatformSupportHost*>(
    ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost());
  auto inprocess = static_cast<DrmGpuPlatformSupportHostInprocess*>(
    support->get_delegate());
  inprocess->OnMessageReceived(*msg);
  delete msg;
}

class FakeGpuProcess {//: public IPC::Sender {
 public:
  FakeGpuProcess(
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner)
      : ui_task_runner_(ui_task_runner) {}
  ~FakeGpuProcess() {} // override {}

  void Init() {
    base::Callback<void(Message*)> sender =
      base::Bind(&DispatchToGpuPlatformSupportHostTask);

    auto platform_support = static_cast<DrmGpuPlatformSupport*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());
    auto inprocess = new DrmGpuPlatformSupportInprocess(platform_support);
    
    inprocess->OnChannelEstablished(ui_task_runner_, sender);
  }

  void InitOnIO() {
    // // IPC::MessageFilter* filter = ui::OzonePlatform::GetInstance()
    // //                                  ->GetGpuPlatformSupport()
    // //                                  ->GetMessageFilter();

    // if (filter)
    //   filter->OnFilterAdded(this);
  }

  // bool Send(IPC::Message* msg) override {
  //   ui_task_runner_->PostTask(
  //       FROM_HERE, base::Bind(&DispatchToGpuPlatformSupportHostTask, msg));
  //   return true;
  // }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
};

static void DispatchToGpuPlatformSupportTask(Message* msg) {
  auto support = static_cast<DrmGpuPlatformSupport*>(
    ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());
  auto inprocess = static_cast<DrmGpuPlatformSupportInprocess*>(support->get_delegate());
  inprocess->OnMessageReceived(*msg);
  delete msg;
}

class FakeGpuProcessHost {
 public:
  FakeGpuProcessHost(
      const scoped_refptr<base::SingleThreadTaskRunner>& gpu_task_runner)
      : gpu_task_runner_(gpu_task_runner) {}
  ~FakeGpuProcessHost() {}

  void Init() {
    base::Callback<void(Message*)> sender =
      base::Bind(&DispatchToGpuPlatformSupportTask);

    LOG(INFO) << "FakeGpuProcessHost::Init";
    auto support = static_cast<DrmGpuPlatformSupportHost*>(
      ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost());
    auto inprocess = static_cast<DrmGpuPlatformSupportHostInprocess*>(
      support->get_delegate());
    inprocess->OnChannelEstablished(kGpuProcessHostId, gpu_task_runner_, sender);
  }

 private:
  scoped_refptr<base::SingleThreadTaskRunner> gpu_task_runner_;
};

OzoneGpuTestHelper::OzoneGpuTestHelper() {
}

OzoneGpuTestHelper::~OzoneGpuTestHelper() {
}

bool OzoneGpuTestHelper::Initialize(
    const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
    const scoped_refptr<base::SingleThreadTaskRunner>& gpu_task_runner) {
  LOG(INFO) << "OzoneGpuTestHelper::Initalize starting";
  
  io_helper_thread_.reset(new base::Thread("IOHelperThread"));
  if (!io_helper_thread_->StartWithOptions(
          base::Thread::Options(base::MessageLoop::TYPE_IO, 0)))
    return false;

  fake_gpu_process_.reset(new FakeGpuProcess(ui_task_runner));
  io_helper_thread_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&FakeGpuProcess::InitOnIO,
                            base::Unretained(fake_gpu_process_.get())));
  fake_gpu_process_->Init();

  fake_gpu_process_host_.reset(new FakeGpuProcessHost(gpu_task_runner));
  fake_gpu_process_host_->Init();

  LOG(INFO) << "OzoneGpuTestHelper::Initalize returning true";

  return true;
}

}  // namespace ui
