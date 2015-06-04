// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/keyboard_native/clip_animation.h"
#include "services/keyboard_native/fast_out_slow_in_interpolator.h"

namespace keyboard {

ClipAnimation::ClipAnimation(const base::TimeTicks& start_ticks,
                             const base::TimeDelta& duration,
                             const gfx::PointF& origin,
                             const float& end_radius)
    : time_interpolator_(make_scoped_ptr(new FastOutSlowInInterpolator())),
      start_ticks_(start_ticks),
      duration_(duration),
      origin_(origin),
      end_radius_(end_radius) {
  DCHECK(!duration.is_zero());
}

ClipAnimation::~ClipAnimation() {
}

// Animation implementation.
void ClipAnimation::Draw(SkCanvas* canvas,
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

  SkPath path;
  path.addCircle(origin_.x(), origin_.y(), end_radius_ * ratio_complete);
  canvas->clipPath(path);
}

bool ClipAnimation::IsDone(const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  return delta_from_start >= duration_;
}

}  // namespace keyboard
