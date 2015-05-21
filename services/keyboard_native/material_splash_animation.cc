// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/keyboard_native/material_splash_animation.h"

namespace keyboard {

MaterialSplashAnimation::MaterialSplashAnimation(
    const base::TimeTicks& start_ticks,
    const base::TimeDelta& duration,
    const gfx::Point& origin)
    : start_ticks_(start_ticks), duration_(duration), origin_(origin) {
  DCHECK(!duration.is_zero());
}

MaterialSplashAnimation::~MaterialSplashAnimation() {
}

// Animation implementation.
void MaterialSplashAnimation::Draw(SkCanvas* canvas,
                                   const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  if (delta_from_start < base::TimeDelta::FromMilliseconds(0)) {
    return;
  }

  if (delta_from_start >= duration_) {
    return;
  }

  float ratio_complete =
      delta_from_start.InMillisecondsF() / duration_.InMillisecondsF();

  int alpha = 0xff * (1.0f - ratio_complete);
  int color = (alpha << 24) + 0xCCCCCC;
  SkPaint paint;
  paint.setColor(color);

  float radius = (190.0f * ratio_complete) + 10.0f;

  canvas->drawCircle(origin_.x(), origin_.y(), radius, paint);
}

bool MaterialSplashAnimation::IsDone(const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  return delta_from_start >= duration_;
}

}  // namespace keyboard
