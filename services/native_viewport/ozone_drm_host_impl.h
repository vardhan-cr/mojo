// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_OZONE_DRM_HOST_IMPL_H_
#define SERVICES_NATIVE_VIEWPORT_OZONE_DRM_HOST_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/ozone_drm_host/public/interfaces/ozone_drm_host.mojom.h"
#include "mojo/services/ozone_drm_gpu/public/interfaces/ozone_drm_gpu.mojom.h"
#include "ui/ozone/platform/drm/host/drm_gpu_platform_support_host.h"

namespace native_viewport {

class OzoneDrmHostImpl : public mojo::OzoneDrmHost,
                         public ui::DrmGpuPlatformSupportHostDelegate {
 public:
  OzoneDrmHostImpl(mojo::InterfaceRequest<mojo::OzoneDrmHost> request,
                   const mojo::OzoneDrmGpuPtr& ozone_drm_gpu);
  ~OzoneDrmHostImpl() override;

  // OzoneDrmHost
  void UpdateNativeDisplays(mojo::Array<mojo::DisplaySnapshotPtr> displays) override;
  void DisplayConfigured(int64_t id, bool result) override;
  
  // DrmGpuPlatformSupportHostDelegate.
  void CreateWindow(const gfx::AcceleratedWidget& widget) override;
  void WindowBoundsChanged(const gfx::AcceleratedWidget& widget,
                           const gfx::Rect& bounds) override;
  void AddGraphicsDevice(const base::FilePath& path,
                         const base::FileDescriptor& fd) override;
  bool RefreshNativeDisplays() override;
  bool ConfigureNativeDisplay(int64_t id,
                              const ui::DisplayMode_Params& mode,
                              const gfx::Point& originhost) override;
  
 private:
  mojo::StrongBinding<mojo::OzoneDrmHost> binding_;
  const mojo::OzoneDrmGpuPtr& ozone_drm_gpu_;
  ui::DrmGpuPlatformSupportHost* platform_support_;
  
  DISALLOW_COPY_AND_ASSIGN(OzoneDrmHostImpl);
};

}  // namespace

#endif  // SERVICES_NATIVE_VIEWPORT_OZONE_DRM_HOST_IMPL_H_
