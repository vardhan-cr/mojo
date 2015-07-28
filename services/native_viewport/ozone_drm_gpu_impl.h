// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_OZONE_DRM_GPU_IMPL_H_
#define SERVICES_NATIVE_VIEWPORT_OZONE_DRM_GPU_IMPL_H_

#include <vector>
#include "base/callback.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/ozone_drm_gpu/public/interfaces/ozone_drm_gpu.mojom.h"
#include "mojo/services/ozone_drm_host/public/interfaces/ozone_drm_host.mojom.h"
#include "ui/ozone/platform/drm/gpu/drm_gpu_platform_support.h"

namespace native_viewport {

class OzoneDrmGpuImpl : public mojo::OzoneDrmGpu,
                        public ui::DrmGpuPlatformSupportDelegate {
 public:
  OzoneDrmGpuImpl(mojo::InterfaceRequest<mojo::OzoneDrmGpu> request,
                  const mojo::OzoneDrmHostPtr& ozone_drm_host,
                  base::Closure device_added_callback);
  ~OzoneDrmGpuImpl() override;

  // OzoneDrmGpu.
  void CreateWindow(int64_t widget) override;
  void WindowBoundsChanged(int64_t widget, mojo::RectPtr bounds) override;
    
  void AddGraphicsDevice(const mojo::String& file_path, int32_t file_descriptor) override;
  void RefreshNativeDisplays() override;
  void ConfigureNativeDisplay(int64_t id, mojo::DisplayModePtr mode, mojo::PointPtr originhost) override;

  // DrmGpuPlatformSupportDelegate
  void UpdateNativeDisplays(
    const std::vector<ui::DisplaySnapshot_Params>& displays) override;
  void DisplayConfigured(int64_t id, bool result) override;

 private:
  mojo::StrongBinding<mojo::OzoneDrmGpu> binding_;
  ui::DrmGpuPlatformSupport* platform_support_;
  const mojo::OzoneDrmHostPtr& ozone_drm_host_;
  base::Closure device_added_callback_;
  
  DISALLOW_COPY_AND_ASSIGN(OzoneDrmGpuImpl);
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_OZONE_DRM_GPU_IMPL_H_
