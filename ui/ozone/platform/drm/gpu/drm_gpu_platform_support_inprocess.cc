// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/location.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_display_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support_inprocess.h"

namespace ui {

DrmGpuPlatformSupportInprocess::DrmGpuPlatformSupportInprocess(DrmGpuPlatformSupport* platform_support)
  : platform_support_(platform_support) {
  platform_support->SetDelegate(this);
}

DrmGpuPlatformSupportInprocess::~DrmGpuPlatformSupportInprocess() {
}

void DrmGpuPlatformSupportInprocess::OnChannelEstablished(
  scoped_refptr<base::SingleThreadTaskRunner> send_runner,
  base::Callback<void(Message*)> send_callback) {
  
  send_runner_ = send_runner;
  send_callback_ = send_callback;

  platform_support_->OnChannelEstablished();
}

bool DrmGpuPlatformSupportInprocess::OnMessageReceived(const Message& message) {
  bool handled = true;
  LOG(INFO) << "DrmGpuPlatformSupportInprocess::OnMessageReceived " << message.id;
  
  switch (message.id) {
    case OZONE_GPU_MSG__CREATE_WINDOW:
      platform_support_->OnCreateWindow(
        static_cast<const OzoneGpuMsg_CreateWindow*>(&message)->widget);
      break;
      
    case OZONE_GPU_MSG__WINDOW_BOUNDS_CHANGED: {
      auto message_params = static_cast<const OzoneGpuMsg_WindowBoundsChanged*>(&message);
      platform_support_->OnWindowBoundsChanged(message_params->widget, message_params->bounds);
      break;
    }

    case OZONE_GPU_MSG__ADD_GRAPHICS_DEVICE: {
      auto message_params = static_cast<const OzoneGpuMsg_AddGraphicsDevice*>(&message);
      platform_support_->OnAddGraphicsDevice(message_params->path, message_params->fd);
      break;
    }

    case OZONE_GPU_MSG__REFRESH_NATIVE_DISPLAYS:
      platform_support_->OnRefreshNativeDisplays();
      break;

    case OZONE_GPU_MSG__CONFIGURE_NATIVE_DISPLAY: {
      auto message_params = static_cast<const OzoneGpuMsg_ConfigureNativeDisplay*>(&message);
      platform_support_->OnConfigureNativeDisplay(message_params->id, message_params->mode, message_params->originhost);
      break;
    }
    
      
  // IPC_BEGIN_MESSAGE_MAP(DrmGpuPlatformSupport, message)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_DestroyWindow, OnDestroyWindow)  
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorSet, OnCursorSet)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorMove, OnCursorMove)

  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_DisableNativeDisplay, OnDisableNativeDisplay)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_TakeDisplayControl, OnTakeDisplayControl)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_RelinquishDisplayControl,
  //                     OnRelinquishDisplayControl)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_RemoveGraphicsDevice, OnRemoveGraphicsDevice)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_GetHDCPState, OnGetHDCPState)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_SetHDCPState, OnSetHDCPState)
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_SetGammaRamp, OnSetGammaRamp);
  // IPC_MESSAGE_HANDLER(OzoneGpuMsg_CheckOverlayCapabilities,
  //                     OnCheckOverlayCapabilities)
  // IPC_MESSAGE_UNHANDLED(handled = false);
  // IPC_END_MESSAGE_MAP()

    default:
      handled = false;
  }
  
//  if (!handled)
//    for (size_t i = 0; i < handlers_.size(); ++i)
//      if (handlers_[i]->OnMessageReceived(message))
//        return true;
  
  return false;
}

void DrmGpuPlatformSupportInprocess::UpdateNativeDisplays(
  const std::vector<DisplaySnapshot_Params>& displays) {
  Send(new OzoneHostMsg_UpdateNativeDisplays(displays));
}

void DrmGpuPlatformSupportInprocess::DisplayConfigured(int64_t id, bool result) {
  Send(new OzoneHostMsg_DisplayConfigured(id, result));
}

bool DrmGpuPlatformSupportInprocess::Send(Message* message) {
  if (//IsConnected() &&
      send_runner_->PostTask(FROM_HERE, base::Bind(send_callback_, message)))
    return true;

  delete message;
  return false;
}

} // namespace

