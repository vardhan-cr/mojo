// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host_inprocess.h"

namespace ui {

DrmGpuPlatformSupportHostInprocess::DrmGpuPlatformSupportHostInprocess(
  DrmGpuPlatformSupportHost* platform_support)
  : platform_support_(platform_support) {
}

DrmGpuPlatformSupportHostInprocess::~DrmGpuPlatformSupportHostInprocess() {
}

void DrmGpuPlatformSupportHostInprocess::OnChannelEstablished(
    int host_id,
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    const base::Callback<void(Message*)>& send_callback) {
  LOG(INFO) << "DrmGpuPlatformSupportHostInprocess::OnChannelEstablished";

  send_runner_ = send_runner;
  send_callback_ = send_callback;

  platform_support_->OnChannelEstablished(host_id);
}

void DrmGpuPlatformSupportHostInprocess::OnChannelDestroyed(int host_id) {
  send_runner_ = nullptr;
  send_callback_.Reset();

  platform_support_->OnChannelDestroyed(host_id);
}

bool DrmGpuPlatformSupportHostInprocess::OnMessageReceived(const Message& message) {
  LOG(INFO) << "DrmGpuPlatformSupportHostInprocess::OnMessageReceived " << message.id;  
  DrmDisplayHostManager* display_manager = platform_support_->get_display_manager();
  DCHECK(display_manager);
  
  bool handled = true;

  switch (message.id) {
    case OZONE_HOST_MSG__UPDATE_NATIVE_DISPLAYS: {
      auto message_params = static_cast<const OzoneHostMsg_UpdateNativeDisplays*>(&message);
      display_manager->OnUpdateNativeDisplays(message_params->displays);
      break;
    }
      
    case OZONE_HOST_MSG__DISPLAY_CONFIGURED: {
      auto message_params = static_cast<const OzoneHostMsg_DisplayConfigured*>(&message);      
      display_manager->OnDisplayConfigured(message_params->id, message_params->result);
      break;
    }
      
//    case OZONE_HOST_MSG__HDCP_STATE_RECEIVED:
//      display_manager->OnHDCPStateReceived();
//      break;
//  IPC_MESSAGE_HANDLER(OzoneHostMsg_HDCPStateUpdated, display_manager, OnHDCPStateUpdated)
//  IPC_MESSAGE_HANDLER(OzoneHostMsg_DisplayControlTaken, display_manager, OnTakeDisplayControl)
//  IPC_MESSAGE_HANDLER(OzoneHostMsg_DisplayControlRelinquished, display_manager,
//                      OnRelinquishDisplayControl)
//  IPC_MESSAGE_UNHANDLED(handled = false)
    default:
      handled = false;
  }
  return handled;
}

bool DrmGpuPlatformSupportHostInprocess::Send(Message* message) {
  if (platform_support_->IsConnected() &&
      send_runner_->PostTask(FROM_HERE, base::Bind(send_callback_, message)))
    return true;

  delete message;
  return false;
}

void DrmGpuPlatformSupportHostInprocess::CreateWindow(const gfx::AcceleratedWidget& widget) {
  Send(new OzoneGpuMsg_CreateWindow(widget));
}

void DrmGpuPlatformSupportHostInprocess::WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                                             const gfx::Rect& bounds) {
  Send(new OzoneGpuMsg_WindowBoundsChanged(widget, bounds));
}

void DrmGpuPlatformSupportHostInprocess::AddGraphicsDevice(
  const base::FilePath& path,
  const base::FileDescriptor& fd) {
  Send(new OzoneGpuMsg_AddGraphicsDevice(path, fd));
}

bool DrmGpuPlatformSupportHostInprocess::RefreshNativeDisplays() {
  return Send(new OzoneGpuMsg_RefreshNativeDisplays());
}

bool DrmGpuPlatformSupportHostInprocess::ConfigureNativeDisplay(
  int64_t id,
  const DisplayMode_Params& mode,
  const gfx::Point& originhost) {
  return Send(new OzoneGpuMsg_ConfigureNativeDisplay(id, mode, originhost));
}



} // namespace
