// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_BITMAP_UPLOADER_BITMAP_UPLOADER_H_
#define EXAMPLES_BITMAP_UPLOADER_BITMAP_UPLOADER_H_

#include <MGL/mgl_types.h>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/services/geometry/public/interfaces/geometry.mojom.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace mojo {
class Shell;
class View;

// BitmapUploader is useful if you want to draw a bitmap or color in a View.
class BitmapUploader : public ResourceReturner {
 public:
  explicit BitmapUploader(View* view);
  ~BitmapUploader() override;

  void Init(Shell* shell);

  // Sets the color which is RGBA.
  void SetColor(uint32_t color);

  enum Format {
    RGBA,  // Pixel layout on Android.
    BGRA,  // Pixel layout everywhere else.
  };

  // Sets a bitmap.
  void SetBitmap(int width,
                 int height,
                 scoped_ptr<std::vector<unsigned char>> data,
                 Format format);

 private:
  void Upload();
  uint32_t BindTextureForSize(const Size size);
  uint32_t TextureFormat();

  void SetIdNamespace(uint32_t id_namespace);

  // ResourceReturner implementation.
  void ReturnResources(Array<ReturnedResourcePtr> resources) override;

  View* view_;
  GpuPtr gpu_service_;
  MGLContext mgl_context_;

  Size size_;
  uint32_t color_;
  int width_;
  int height_;
  Format format_;
  scoped_ptr<std::vector<unsigned char>> bitmap_;
  SurfacePtr surface_;
  Size surface_size_;
  uint32_t next_resource_id_;
  uint32_t id_namespace_;
  uint32_t local_id_;
  base::hash_map<uint32_t, uint32_t> resource_to_texture_id_map_;
  mojo::Binding<mojo::ResourceReturner> returner_binding_;

  DISALLOW_COPY_AND_ASSIGN(BitmapUploader);
};

}  // namespace mojo

#endif  // EXAMPLES_BITMAP_UPLOADER_BITMAP_UPLOADER_H_
