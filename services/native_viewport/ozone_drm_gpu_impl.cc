// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/file_descriptor_posix.h"
#include "base/logging.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "services/native_viewport/ozone_drm_gpu_impl.h"
#include "ui/ozone/public/ozone_platform.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_display_manager.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

namespace native_viewport {
  
OzoneDrmGpuImpl::OzoneDrmGpuImpl(mojo::InterfaceRequest<mojo::OzoneDrmGpu> request,
                                 const mojo::OzoneDrmHostPtr& ozone_drm_host,
                                 base::Closure device_added_callback)
  : binding_(this, request.Pass()),
    ozone_drm_host_(ozone_drm_host),
    device_added_callback_(device_added_callback) {
  LOG(INFO) << "OzoneDrmGpuImpl()";
  
  ui::OzonePlatform::InitializeForGPU();

  platform_support_ = static_cast<ui::DrmGpuPlatformSupport*>(
    ui::OzonePlatform::GetInstance()->GetGpuPlatformSupport());
  
  platform_support_->SetDelegate(this);
  platform_support_->OnChannelEstablished();
}

OzoneDrmGpuImpl::~OzoneDrmGpuImpl() {
}

void OzoneDrmGpuImpl::CreateWindow(int64_t widget) {
  LOG(INFO) << "OzoneDrmGpuImpl::CreateWindow " << widget;
  platform_support_->OnCreateWindow(widget);
}

void OzoneDrmGpuImpl::WindowBoundsChanged(int64_t widget, mojo::RectPtr bounds) {
  LOG(INFO) << "OzoneDrmGpuImpl::WindowBoundsChanged " << widget << " " << bounds.To<gfx::Rect>().ToString();
  platform_support_->OnWindowBoundsChanged(widget, bounds.To<gfx::Rect>());
}

void OzoneDrmGpuImpl::AddGraphicsDevice(const mojo::String& file_path, int32_t file_descriptor) {
  LOG(INFO) << "OzoneDrmGpuImpl::AddGraphicsDevice " << file_path.get() << " fd " << file_descriptor;
  platform_support_->OnAddGraphicsDevice(base::FilePath(file_path.get()),
                                         base::FileDescriptor(file_descriptor, false));
  device_added_callback_.Run();
}

void OzoneDrmGpuImpl::RefreshNativeDisplays() {
  LOG(INFO) << "OzoneDrmGpuImpl::RefreshNativeDisplays";
  platform_support_->OnRefreshNativeDisplays();  
}

void OzoneDrmGpuImpl::ConfigureNativeDisplay(int64_t id,
                                             mojo::DisplayModePtr mode,
                                             mojo::PointPtr originhost) {
  LOG(INFO) << "OzoneDrmGpuImpl::ConfigureNativeDisplay ";
  platform_support_->OnConfigureNativeDisplay(
    id, mode.To<ui::DisplayMode_Params>(), originhost.To<gfx::Point>());  
}

void OzoneDrmGpuImpl::UpdateNativeDisplays(
  const std::vector<ui::DisplaySnapshot_Params>& displays) {
  LOG(INFO) << "OzoneDrmGpuImpl::UpdateNativeDisplays";

  for (size_t i = 0; i < displays.size(); ++i) {
    LOG(INFO) << "Display " << i << ": id " << displays[i].display_id;
    LOG(INFO) << "native mode " << displays[i].native_mode.size.width() << "x" << displays[i].native_mode.size.height() << "x" << displays[i].native_mode.refresh_rate;
    LOG(INFO) << "is_interlaced " << displays[i].native_mode.is_interlaced;
  }
  
  ozone_drm_host_->UpdateNativeDisplays(
    mojo::Array<mojo::DisplaySnapshotPtr>::From(displays));
}

void OzoneDrmGpuImpl::DisplayConfigured(int64_t id, bool result) {
  LOG(INFO) << "OzoneDrmGpuImpl::DisplayConfigured";
  ozone_drm_host_->DisplayConfigured(id, result);
}

} // namespace


