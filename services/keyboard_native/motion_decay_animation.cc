// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/keyboard_native/motion_decay_animation.h"

namespace keyboard {

MotionDecayAnimation::MotionDecayAnimation(const base::TimeTicks& start_ticks,
                                           const base::TimeDelta& duration,
                                           const gfx::Point& start,
                                           const gfx::Point& end)
    : start_ticks_(start_ticks), duration_(duration), start_(start), end_(end) {
  DCHECK(!duration.is_zero());
}

MotionDecayAnimation::~MotionDecayAnimation() {
}

// Animation implementation.
void MotionDecayAnimation::Draw(SkCanvas* canvas,
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

  SkPath path;
  path.moveTo(static_cast<float>(start_.x()), static_cast<float>(start_.y()));
  path.lineTo(static_cast<float>(end_.x()), static_cast<float>(end_.y()));

  int stroke_width = 25.0f * (1.0f - ratio_complete);

  SkPaint paint;
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setStrokeJoin(SkPaint::kRound_Join);
  paint.setStrokeWidth(stroke_width);
  paint.setColor(0xFF0000FF);

  canvas->drawPath(path, paint);
}

bool MotionDecayAnimation::IsDone(const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  return delta_from_start >= duration_;
}

}  // namespace keyboard
