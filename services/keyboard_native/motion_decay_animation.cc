// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "services/keyboard_native/motion_decay_animation.h"

namespace keyboard {

MotionDecayAnimation::MotionDecayAnimation(const base::TimeTicks& start_ticks,
                                           const base::TimeDelta& duration,
                                           const gfx::PointF& start,
                                           const gfx::PointF& end)
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
  path.moveTo(start_.x(), start_.y());
  path.lineTo(end_.x(), end_.y());

  int stroke_width = 25.0f * (1.0f - ratio_complete);

  SkPaint paint;
  paint.setStyle(SkPaint::kStroke_Style);
  paint.setStrokeCap(SkPaint::kRound_Cap);
  paint.setStrokeJoin(SkPaint::kRound_Join);
  paint.setStrokeWidth(stroke_width);
  paint.setColor(SkColorSetRGB(0x00, 0x00, 0xff));

  canvas->drawPath(path, paint);
}

bool MotionDecayAnimation::IsDone(const base::TimeTicks& current_ticks) {
  const base::TimeDelta delta_from_start = current_ticks - start_ticks_;
  return delta_from_start >= duration_;
}

}  // namespace keyboard
