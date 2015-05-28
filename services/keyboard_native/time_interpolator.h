// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_TIME_INTERPOLATOR_H_
#define SERVICES_KEYBOARD_NATIVE_TIME_INTERPOLATOR_H_

namespace keyboard {

// NOTE: This class has been translated to C++ from the Android Open Source
// Project.  Specifically from the following files:
// https://github.com/android/platform_frameworks_base/blob/3bdbf644d61f46b531838558fabbd5b990fc4913/core/java/android/animation/TimeInterpolator.java

// A time interpolator defines the rate of change of an animation. This allows
// animations to have non-linear motion, such as acceleration and deceleration.
class TimeInterpolator {
 public:
  virtual ~TimeInterpolator() {}

  // Maps a value representing the elapsed fraction of an animation to a value
  // that represents the interpolated fraction.
  //
  // input - A value between 0 and 1.0 indicating our current point in the
  // animation where 0 represents the start and 1.0 represents the end
  //
  // returns: The interpolation value. This value can be more than 1.0 for
  // interpolators which overshoot their targets, or less than 0 for
  // interpolators that undershoot their targets.
  virtual float GetInterpolation(float input) = 0;
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_TIME_INTERPOLATOR_H_
