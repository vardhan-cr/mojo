// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_MATERIAL_SPLASH_ANIMATION_H_
#define SERVICES_KEYBOARD_NATIVE_MATERIAL_SPLASH_ANIMATION_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "services/keyboard_native/animation.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/geometry/point_f.h"

namespace keyboard {

class TimeInterpolator;

class MaterialSplashAnimation : public Animation {
 public:
  // Creates a splash animation starting at |start_ticks| lasting |duration| at
  // |origin|.  |duration| must be non-zero.
  MaterialSplashAnimation(const base::TimeTicks& start_ticks,
                          const base::TimeDelta& duration,
                          const gfx::PointF& origin);
  ~MaterialSplashAnimation() override;

  // Animation implementation.
  void Draw(SkCanvas* canvas, const base::TimeTicks& current_ticks) override;
  bool IsDone(const base::TimeTicks& current_ticks) override;

 private:
  scoped_ptr<TimeInterpolator> time_interpolator_;
  const base::TimeTicks start_ticks_;
  const base::TimeDelta duration_;
  const gfx::PointF origin_;

  DISALLOW_COPY_AND_ASSIGN(MaterialSplashAnimation);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_MATERIAL_SPLASH_ANIMATION_H_
