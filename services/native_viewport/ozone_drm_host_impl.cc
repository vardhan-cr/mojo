// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/process/process.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/ozone_drm_gpu/ozone_drm_gpu_type_converters.h"
#include "services/native_viewport/ozone_drm_host_impl.h"
#include "ui/ozone/public/ozone_platform.h"


namespace native_viewport {

OzoneDrmHostImpl::OzoneDrmHostImpl(mojo::InterfaceRequest<mojo::OzoneDrmHost> request,
                                   const mojo::OzoneDrmGpuPtr& ozone_drm_gpu)
  : binding_(this, request.Pass()),
    ozone_drm_gpu_(ozone_drm_gpu),
    platform_support_(static_cast<ui::DrmGpuPlatformSupportHost*>(
                        ui::OzonePlatform::GetInstance()->GetGpuPlatformSupportHost())) {
  LOG(INFO) << "OzoneDrmHostImpl()";  

  platform_support_->SetDelegate(this);
  platform_support_->OnChannelEstablished(base::Process::Current().Pid());
}

OzoneDrmHostImpl::~OzoneDrmHostImpl() {
}

void OzoneDrmHostImpl::UpdateNativeDisplays(mojo::Array<mojo::DisplaySnapshotPtr> displays) {
  platform_support_->get_display_manager()->OnUpdateNativeDisplays(
    displays.To<std::vector<ui::DisplaySnapshot_Params>>());
}

void OzoneDrmHostImpl::DisplayConfigured(int64_t id, bool result) {
  platform_support_->get_display_manager()->OnDisplayConfigured(id, result);
}

void OzoneDrmHostImpl::CreateWindow(const gfx::AcceleratedWidget& widget) {
  ozone_drm_gpu_->CreateWindow(widget);
}

void OzoneDrmHostImpl::WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                           const gfx::Rect& bounds) {
  ozone_drm_gpu_->WindowBoundsChanged(widget, mojo::Rect::From(bounds));
}

void OzoneDrmHostImpl::AddGraphicsDevice(const base::FilePath& path,
                                         const base::FileDescriptor& fd) {
  ozone_drm_gpu_->AddGraphicsDevice(path.value(), fd.fd);
}

bool OzoneDrmHostImpl::RefreshNativeDisplays() {
  ozone_drm_gpu_->RefreshNativeDisplays();
  return true;
}

bool OzoneDrmHostImpl::ConfigureNativeDisplay(int64_t id,
                                              const ui::DisplayMode_Params& mode,
                                              const gfx::Point& originhost) {
  LOG(INFO) << "ConfigureNativeDisplay id " << id;
  ozone_drm_gpu_->ConfigureNativeDisplay(
    id, mojo::DisplayMode::From<ui::DisplayMode_Params>(mode), mojo::Point::From(originhost));
  return true;
}

} // namespace


