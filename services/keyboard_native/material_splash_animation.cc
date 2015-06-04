// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/keyboard_native/fast_out_slow_in_interpolator.h"
#include "services/keyboard_native/material_splash_animation.h"

namespace keyboard {

MaterialSplashAnimation::MaterialSplashAnimation(
    const base::TimeTicks& start_ticks,
    const base::TimeDelta& duration,
    const gfx::PointF& origin)
    : time_interpolator_(make_scoped_ptr(new FastOutSlowInInterpolator())),
      start_ticks_(start_ticks),
      duration_(duration),
      origin_(origin) {
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

  float ratio_complete = time_interpolator_->GetInterpolation(
      delta_from_start.InMillisecondsF() / duration_.InMillisecondsF());

  int alpha = 0xff * (1.0f - ratio_complete);
  SkPaint paint;
  paint.setColor(SkColorSetARGB(alpha, 0x88, 0x88, 0x88));

  float radius = (190.0f * ratio_complete) + 10.0f;

  canvas->drawCircle(origin_.x(), origin_.y(), radius, paint);
}

bool MaterialSplashAnimation::IsDone(const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  return delta_from_start >= duration_;
}

}  // namespace keyboard
