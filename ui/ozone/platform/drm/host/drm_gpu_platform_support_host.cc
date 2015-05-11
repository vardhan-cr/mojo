// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

#include "base/trace_event/trace_event.h"
//#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
//#include "ui/ozone/common/gpu/ozone_gpu_messages.h"
#include "ui/ozone/platform/drm/host/channel_observer.h"
#include "ui/ozone/platform/drm/host/drm_cursor.h"

namespace ui {

DrmGpuPlatformSupportHost::DrmGpuPlatformSupportHost(DrmCursor* cursor)
  : display_manager_(nullptr),
    delegate_(nullptr) {
  //: cursor_(cursor) {
}

DrmGpuPlatformSupportHost::~DrmGpuPlatformSupportHost() {
}

void DrmGpuPlatformSupportHost::SetDisplayManager(DrmDisplayHostManager* display_manager) {
  DCHECK(!display_manager_);
  display_manager_ = display_manager;
  LOG(INFO) << "SetDisplayManager " << display_manager;
  
  if (IsConnected()) {
    display_manager_->OnChannelEstablished(host_id_);
  }
}

// void DrmGpuPlatformSupportHost::RegisterHandler(
//     GpuPlatformSupportHost* handler) {
//   handlers_.push_back(handler);

//   // if (IsConnected())
//   //   handler->OnChannelEstablished(host_id_, send_runner_, send_callback_);
// }

// void DrmGpuPlatformSupportHost::UnregisterHandler(
//     GpuPlatformSupportHost* handler) {
//   std::vector<GpuPlatformSupportHost*>::iterator it =
//       std::find(handlers_.begin(), handlers_.end(), handler);
//   if (it != handlers_.end())
//     handlers_.erase(it);
// }

void DrmGpuPlatformSupportHost::AddChannelObserver(ChannelObserver* observer) {
  channel_observers_.AddObserver(observer);

  if (IsConnected())
    observer->OnChannelEstablished();
}

void DrmGpuPlatformSupportHost::RemoveChannelObserver(
    ChannelObserver* observer) {
  channel_observers_.RemoveObserver(observer);
}

bool DrmGpuPlatformSupportHost::IsConnected() {
  return host_id_ >= 0;
}

void DrmGpuPlatformSupportHost::OnChannelEstablished(int host_id) {
  TRACE_EVENT1("drm", "DrmGpuPlatformSupportHost::OnChannelEstablished",
               "host_id", host_id);
  
  host_id_ = host_id;

  LOG(INFO) << "OnChannelEstablished display_manager_ " << display_manager_;
  
  if (display_manager_) {
    display_manager_->OnChannelEstablished(host_id_);
  }
  
  FOR_EACH_OBSERVER(ChannelObserver, channel_observers_,
                    OnChannelEstablished());
}

void DrmGpuPlatformSupportHost::OnChannelDestroyed(int host_id) {
   TRACE_EVENT1("drm", "DrmGpuPlatformSupportHost::OnChannelDestroyed",
                "host_id", host_id);
//   cursor_->OnChannelDestroyed(host_id);

   if (host_id_ == host_id) {
     host_id_ = -1;
     FOR_EACH_OBSERVER(ChannelObserver, channel_observers_,
                       OnChannelDestroyed());
   }

//   for (size_t i = 0; i < handlers_.size(); ++i)
//     handlers_[i]->OnChannelDestroyed(host_id);
}

void DrmGpuPlatformSupportHost::CreateWindow(const gfx::AcceleratedWidget& widget) {
  DCHECK(delegate_);
  delegate_->CreateWindow(widget);
}

void DrmGpuPlatformSupportHost::WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                                     const gfx::Rect& bounds) {
  DCHECK(delegate_);
  delegate_->WindowBoundsChanged(widget, bounds);
}

void DrmGpuPlatformSupportHost::AddGraphicsDevice(const base::FilePath& path,
                                                  const base::FileDescriptor& fd) {
  DCHECK(delegate_);
  delegate_->AddGraphicsDevice(path, fd);
}

bool DrmGpuPlatformSupportHost::RefreshNativeDisplays() {
  DCHECK(delegate_);
  return delegate_->RefreshNativeDisplays();
}

bool DrmGpuPlatformSupportHost::ConfigureNativeDisplay(int64_t id,
                                                       const DisplayMode_Params& mode,
                                                       const gfx::Point& originhost) {
  DCHECK(delegate_);
  return delegate_->ConfigureNativeDisplay(id, mode, originhost);
}


}  // namespace ui
