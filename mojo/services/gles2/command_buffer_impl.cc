// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/services/gles2/command_buffer_impl.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "mojo/services/gles2/command_buffer_driver.h"

namespace mojo {
namespace {
void DestroyDriver(scoped_ptr<CommandBufferDriver> driver) {
  // Just let ~scoped_ptr run.
}

void RunCallback(const Callback<void()>& callback) {
  callback.Run();
}

class CommandBufferDriverClientImpl : public CommandBufferDriver::Client {
 public:
  CommandBufferDriverClientImpl(
      base::WeakPtr<CommandBufferImpl> command_buffer,
      scoped_refptr<base::SingleThreadTaskRunner> control_task_runner)
      : command_buffer_(command_buffer),
        control_task_runner_(control_task_runner) {}

 private:
  void DidDestroy() override { delete this; }

  void UpdateVSyncParameters(base::TimeTicks timebase,
                             base::TimeDelta interval) override {
    control_task_runner_->PostTask(
        FROM_HERE, base::Bind(&CommandBufferImpl::UpdateVSyncParameters,
                              command_buffer_, timebase, interval));
  }

  void LostContext(int32_t lost_reason) override {
    control_task_runner_->PostTask(
        FROM_HERE, base::Bind(&CommandBufferImpl::LostContext, command_buffer_,
                              lost_reason));
  }

  base::WeakPtr<CommandBufferImpl> command_buffer_;
  scoped_refptr<base::SingleThreadTaskRunner> control_task_runner_;
};
}

CommandBufferImpl::CommandBufferImpl(
    InterfaceRequest<CommandBuffer> request,
    ViewportParameterListenerPtr listener,
    scoped_refptr<base::SingleThreadTaskRunner> control_task_runner,
    gpu::SyncPointManager* sync_point_manager,
    scoped_ptr<CommandBufferDriver> driver)
    : sync_point_manager_(sync_point_manager),
      driver_task_runner_(base::MessageLoop::current()->task_runner()),
      driver_(driver.Pass()),
      viewport_parameter_listener_(listener.Pass()),
      binding_(this),
      weak_factory_(this) {
  driver_->set_client(new CommandBufferDriverClientImpl(
      weak_factory_.GetWeakPtr(), control_task_runner));

  control_task_runner->PostTask(
      FROM_HERE, base::Bind(&CommandBufferImpl::BindToRequest,
                            base::Unretained(this), base::Passed(&request)));
}

CommandBufferImpl::~CommandBufferImpl() {
  binding_.client()->DidDestroy();
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&DestroyDriver, base::Passed(&driver_)));
}

void CommandBufferImpl::Initialize(
    CommandBufferSyncClientPtr sync_client,
    CommandBufferSyncPointClientPtr sync_point_client,
    ScopedSharedBufferHandle shared_state) {
  sync_point_client_ = sync_point_client.Pass();
  driver_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&CommandBufferDriver::Initialize,
                 base::Unretained(driver_.get()), base::Passed(&sync_client),
                 base::Passed(&shared_state)));
}

void CommandBufferImpl::SetGetBuffer(int32_t buffer) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::SetGetBuffer,
                            base::Unretained(driver_.get()), buffer));
}

void CommandBufferImpl::Flush(int32_t put_offset) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::Flush,
                            base::Unretained(driver_.get()), put_offset));
}

void CommandBufferImpl::MakeProgress(int32_t last_get_offset) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::MakeProgress,
                            base::Unretained(driver_.get()), last_get_offset));
}

void CommandBufferImpl::RegisterTransferBuffer(
    int32_t id,
    ScopedSharedBufferHandle transfer_buffer,
    uint32_t size) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::RegisterTransferBuffer,
                            base::Unretained(driver_.get()), id,
                            base::Passed(&transfer_buffer), size));
}

void CommandBufferImpl::DestroyTransferBuffer(int32_t id) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&CommandBufferDriver::DestroyTransferBuffer,
                            base::Unretained(driver_.get()), id));
}

void CommandBufferImpl::InsertSyncPoint(bool retire) {
  uint32_t sync_point = sync_point_manager_->GenerateSyncPoint();
  sync_point_client_->DidInsertSyncPoint(sync_point);
  if (retire) {
    driver_task_runner_->PostTask(
        FROM_HERE, base::Bind(&gpu::SyncPointManager::RetireSyncPoint,
                              sync_point_manager_, sync_point));
  }
}

void CommandBufferImpl::RetireSyncPoint(uint32_t sync_point) {
  driver_task_runner_->PostTask(
      FROM_HERE, base::Bind(&gpu::SyncPointManager::RetireSyncPoint,
                            sync_point_manager_, sync_point));
}

void CommandBufferImpl::Echo(const Callback<void()>& callback) {
  driver_task_runner_->PostTaskAndReply(FROM_HERE, base::Bind(&base::DoNothing),
                                        base::Bind(&RunCallback, callback));
}

void CommandBufferImpl::BindToRequest(InterfaceRequest<CommandBuffer> request) {
  binding_.Bind(request.Pass());
}

void CommandBufferImpl::LostContext(int32_t reason) {
  binding_.client()->LostContext(reason);
}

void CommandBufferImpl::UpdateVSyncParameters(base::TimeTicks timebase,
                                              base::TimeDelta interval) {
  if (!viewport_parameter_listener_)
    return;
  viewport_parameter_listener_->OnVSyncParametersUpdated(
      timebase.ToInternalValue(), interval.ToInternalValue());
}

}  // namespace mojo
