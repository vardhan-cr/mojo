// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_SKIA_UTIL_H_
#define UI_GFX_SKIA_UTIL_H_

#include <string>
#include <vector>

#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/skia/include/core/SkRect.h"
#include "third_party/skia/include/core/SkShader.h"
#include "ui/gfx/gfx_export.h"

class SkBitmap;
class SkDrawLooper;

namespace gfx {

class ImageSkiaRep;
class Rect;
class RectF;
class ShadowValue;
class Transform;

// Convert between Skia and gfx rect types.
GFX_EXPORT SkRect RectToSkRect(const Rect& rect);
GFX_EXPORT SkIRect RectToSkIRect(const Rect& rect);
GFX_EXPORT Rect SkIRectToRect(const SkIRect& rect);
GFX_EXPORT SkRect RectFToSkRect(const RectF& rect);
GFX_EXPORT RectF SkRectToRectF(const SkRect& rect);

GFX_EXPORT void TransformToFlattenedSkMatrix(const gfx::Transform& transform,
                                             SkMatrix* flattened);

// Creates a vertical gradient shader. The caller owns the shader.
// Example usage to avoid leaks:
GFX_EXPORT skia::RefPtr<SkShader> CreateGradientShader(int start_point,
                                                       int end_point,
                                                       SkColor start_color,
                                                       SkColor end_color);

// Returns true if the two bitmaps contain the same pixels.
GFX_EXPORT bool BitmapsAreEqual(const SkBitmap& bitmap1,
                                const SkBitmap& bitmap2);

// Converts Skia ARGB format pixels in |skia| to RGBA.
GFX_EXPORT void ConvertSkiaToRGBA(const unsigned char* skia,
                                  int pixel_width,
                                  unsigned char* rgba);

}  // namespace gfx

#endif  // UI_GFX_SKIA_UTIL_H_
