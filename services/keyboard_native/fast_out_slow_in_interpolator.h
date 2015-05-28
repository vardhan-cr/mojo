// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_FAST_OUT_SLOW_IN_INTERPOLATOR_H_
#define SERVICES_KEYBOARD_NATIVE_FAST_OUT_SLOW_IN_INTERPOLATOR_H_

#include "services/keyboard_native/time_interpolator.h"

// NOTE: This class has been translated to C++ from the Android Open Source
// Project.  Specifically from the following files:
// https://github.com/android/platform_frameworks_support/blob/master/v4/java/android/support/v4/view/animation/FastOutSlowInInterpolator.java
// https://github.com/android/platform_frameworks_support/blob/master/v4/java/android/support/v4/view/animation/LookupTableInterpolator.java

namespace keyboard {

// Uses a lookup table for the Bezier curve from (0,0) to (1,1) with control
// points:
// P0 (0, 0)
// P1 (0.4, 0)
// P2 (0.2, 1.0)
// P3 (1.0, 1.0)
class FastOutSlowInInterpolator : public TimeInterpolator {
 public:
  FastOutSlowInInterpolator();
  ~FastOutSlowInInterpolator() override;
  float GetInterpolation(float input) override;

 private:
  float step_size_;

  DISALLOW_COPY_AND_ASSIGN(FastOutSlowInInterpolator);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_FAST_OUT_SLOW_IN_INTERPOLATOR_H_
