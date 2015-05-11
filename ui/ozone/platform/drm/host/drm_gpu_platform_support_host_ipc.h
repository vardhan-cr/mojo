// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_IPC_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_IPC_H_

#include "ui/ozone/platform/drm/host/gpu_platform_support_host.h"
#include "ipc/ipc_sender.h"
#include "ipc/ipc_receiver.h"

namespace ui {

class DrmGpuPlatformSupportHostIpc : public DrmGpuPlatformSupportHost,
                                     public IPC::Sender,
                                     public IPC::Receiver {
 public:
  DrmGpuPlatformSupportHost(DrmCursor* cursor);
  ~DrmGpuPlatformSupportHost() override;

  void OnChannelEstablished(
    int host_id,
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    const base::Callback<void(IPC::Message*)>& send_callback);
  void OnChannelDestroyed(int host_id) override;

  // IPC::Listener:
  bool OnMessageReceived(const IPC::Message& message) override;

  // IPC::Sender:
  bool Send(IPC::Message* message) override;

  void CreateWindow(const gfx::AcceleratedWidget& widget) override;
  void WindowBoundsChanged(const gfx::Rect& bounds,
                           const gfx::AcceleratedWidget& widget) override;

 private:
  scoped_refptr<base::SingleThreadTaskRunner> send_runner_;
  base::Callback<void(IPC::Message*)> send_callback_;
};

}  // namespace ui

#endif  // UI_OZONE_GPU_DRM_GPU_PLATFORM_SUPPORT_HOST_IPC_H_
