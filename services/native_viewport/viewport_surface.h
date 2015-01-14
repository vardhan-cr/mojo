// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_VIEWPORT_SURFACE_H_
#define SERVICES_NATIVE_VIEWPORT_VIEWPORT_SURFACE_H_

#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "cc/surfaces/surface_id.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces_service.mojom.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gfx/size.h"

namespace native_viewport {

// This manages the surface that draws to a particular NativeViewport instance.
class ViewportSurface : public mojo::SurfaceClient {
 public:
  ViewportSurface(mojo::SurfacePtr surface,
                  mojo::Gpu* gpu_service,
                  const gfx::Size& size,
                  cc::SurfaceId child_id);
  ~ViewportSurface() override;

  void SetWidgetId(uint64_t widget_id);
  void SetSize(const gfx::Size& size);
  void SetChildId(cc::SurfaceId child_id);

 private:
  void BindSurfaceToNativeViewport();
  void SubmitFrame();

  // SurfaceClient implementation.
  void SetIdNamespace(uint32_t id_namespace) override;
  void ReturnResources(
      mojo::Array<mojo::ReturnedResourcePtr> resources) override;

  mojo::SurfacePtr surface_;
  mojo::Gpu* gpu_service_;
  uint64_t widget_id_;
  gfx::Size size_;
  bool gles2_bound_surface_created_;
  cc::SurfaceId child_id_;
  base::WeakPtrFactory<ViewportSurface> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ViewportSurface);
};

}  // namespace native_viewport

#endif  // SERVICES_NATIVE_VIEWPORT_VIEWPORT_SURFACE_H_
