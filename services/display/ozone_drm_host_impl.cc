// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/process/process.h"
#include "services/display/ozone_drm_host_impl.h"
#include "ui/ozone/public/ozone_platform.h"

namespace display {

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

void OzoneDrmHostImpl::CreateWindow(const gfx::AcceleratedWidget& widget) {
  ozone_drm_gpu_->CreateWindow();
}

void OzoneDrmHostImpl::WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                                           const gfx::Rect& bounds) {
}

void OzoneDrmHostImpl::AddGraphicsDevice(const base::FilePath& path,
                                         const base::FileDescriptor& fd) {
  ozone_drm_gpu_->AddGraphicsDevice(path.value(), fd.fd);
}

bool OzoneDrmHostImpl::RefreshNativeDisplays() {
  return false;
}

bool OzoneDrmHostImpl::ConfigureNativeDisplay(int64_t id,
                                              const ui::DisplayMode_Params& mode,
                                              const gfx::Point& originhost) {
  return false;
}

} // namespace


