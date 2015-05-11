// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_H_
#define UI_OZONE_PLATFORM_DRM_HOST_DRM_GPU_PLATFORM_SUPPORT_HOST_H_

//#include <vector>

#include "base/callback.h"
#include "base/observer_list.h"
#include "base/file_descriptor_posix.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/public/gpu_platform_support_host.h"
#include "ui/ozone/platform/drm/host/drm_display_host_manager.h"

class SkBitmap;

namespace gfx {
class Point;
}

namespace ui {

class ChannelObserver;
class DrmCursor;

class DrmGpuPlatformSupportHostDelegate {
public:
  DrmGpuPlatformSupportHostDelegate() {}
  virtual ~DrmGpuPlatformSupportHostDelegate() {}

  virtual void CreateWindow(const gfx::AcceleratedWidget& widget) = 0;
  virtual void WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                   const gfx::Rect& bounds) = 0;
  virtual void AddGraphicsDevice(const base::FilePath& path,
                                 const base::FileDescriptor& fd) = 0;
  virtual bool RefreshNativeDisplays() = 0;
  virtual bool ConfigureNativeDisplay(int64_t id,
                                      const DisplayMode_Params& mode,
                                      const gfx::Point& originhost) = 0;  
};

  
class DrmGpuPlatformSupportHost : public GpuPlatformSupportHost {
 public:
  DrmGpuPlatformSupportHost(DrmCursor* cursor);
  ~DrmGpuPlatformSupportHost() override;

  void SetDisplayManager(DrmDisplayHostManager* display_manager);  

  DrmDisplayHostManager* get_display_manager() {
    return display_manager_;
  }

  void SetDelegate(DrmGpuPlatformSupportHostDelegate* delegate) {
    DCHECK(!delegate_);
    delegate_ = delegate;
  }

  DrmGpuPlatformSupportHostDelegate* get_delegate() {
    return delegate_;
  }
    
  //void RegisterHandler(GpuPlatformSupportHost* handler);
  //void UnregisterHandler(GpuPlatformSupportHost* handler);

  void AddChannelObserver(ChannelObserver* observer);
  void RemoveChannelObserver(ChannelObserver* observer);

  bool IsConnected();

  // GpuPlatformSupportHost:
  void OnChannelEstablished(int host_id) override;
  void OnChannelDestroyed(int host_id) override;

  // host to gpu interfaces
  void CreateWindow(const gfx::AcceleratedWidget& widget);
  void WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                           const gfx::Rect& bounds);
  void AddGraphicsDevice(const base::FilePath& path,
                         const base::FileDescriptor& fd);
  bool RefreshNativeDisplays();
  bool ConfigureNativeDisplay(int64_t id,
                              const DisplayMode_Params& mode,
                              const gfx::Point& originhost);
  
 private:
  int host_id_ = -1;

  //std::vector<GpuPlatformSupportHost*> handlers_;  // Not owned.
  DrmDisplayHostManager* display_manager_;           // Not owned
  //DrmCursor* cursor_;                              // Not owned.
  base::ObserverList<ChannelObserver> channel_observers_;
  DrmGpuPlatformSupportHostDelegate* delegate_;
  
};

}  // namespace ui

#endif  // UI_OZONE_GPU_DRM_GPU_PLATFORM_SUPPORT_HOST_H_
