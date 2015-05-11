// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host_ipc.h"
#include "ui/common/gpu/ozone_gpu_messages.h"

namespace ui {

void DrmGpuPlatformSupportHostIpc::OnChannelEstablished(
    int host_id,
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    const base::Callback<void(IPC::Message*)>& send_callback) {

  DrmGpuPlatformSupportHost::OnChannelEstablished(host_id);
  
  send_runner_ = send_runner;
  send_callback_ = send_callback;
  
//  for (size_t i = 0; i < handlers_.size(); ++i)
//    handlers_[i]->OnChannelEstablished(host_id, send_runner_, send_callback_);
}

void DrmGpuPlatformSupportHostIpc::OnChannelDestroyed(int host_id) {
   if (host_id_ == host_id) {
     send_runner_ = nullptr;
     send_callback_.Reset();
   }
   DrmGpuPlatformSupportHost::OnChannelDestroyed(host_id);
}

void DrmGpuPlatformSupportHostIpc::CreateWindow(const gfx::AcceleratedWidget& widget) {
  Send(new OzoneGpuMsg_CreateWindow(widget));
}

void DrmGpuPlatformSupportHostIpc::WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                                       const gfx::Rect& bounds) {
  Send(new OzoneGpuMsg_WindowBoundsChanged(widget, bounds));
}

bool DrmGpuPlatformSupportHostIpc::OnMessageReceived(const IPC::Message& message) {
//  for (size_t i = 0; i < handlers_.size(); ++i)
//    if (handlers_[i]->OnMessageReceived(message))
//      return true;
  DrmDisplayHostManager* display_manager = get_display_manager();
  DCHECK(display_manager);
  
  bool handled = true;
  
  IPC_BEGIN_MESSAGE_MAP(DrmDisplayHostManager, message)
  IPC_MESSAGE_FORWARD(OzoneHostMsg_UpdateNativeDisplays, display_manager, OnUpdateNativeDisplays)
  IPC_MESSAGE_HANDLER(OzoneHostMsg_DisplayConfigured, display_manager, OnDisplayConfigured)
  IPC_MESSAGE_HANDLER(OzoneHostMsg_HDCPStateReceived, display_manager, OnHDCPStateReceived)
  IPC_MESSAGE_HANDLER(OzoneHostMsg_HDCPStateUpdated, display_manager, OnHDCPStateUpdated)
  IPC_MESSAGE_HANDLER(OzoneHostMsg_DisplayControlTaken, display_manager, OnTakeDisplayControl)
  IPC_MESSAGE_HANDLER(OzoneHostMsg_DisplayControlRelinquished, display_manager,
                      OnRelinquishDisplayControl)
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

bool DrmGpuPlatformSupportHostIpc::Send(IPC::Message* message) {
  if (IsConnected() &&
      send_runner_->PostTask(FROM_HERE, base::Bind(send_callback_, message)))
    return true;

  delete message;
  return false;
}

} // namespace
