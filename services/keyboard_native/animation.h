// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_ANIMATION_H_
#define SERVICES_KEYBOARD_NATIVE_ANIMATION_H_

#include "base/time/time.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/geometry/point.h"

namespace keyboard {

// The base class for all keyboard animations.
class Animation {
 public:
  virtual ~Animation() {}

  // Draws the Animation to the |canvas| for the |current_ticks|.
  virtual void Draw(SkCanvas* canvas, const base::TimeTicks& current_ticks) = 0;

  // Returns true if the Animation is finished at |current_ticks|.
  virtual bool IsDone(const base::TimeTicks& current_ticks) = 0;
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_ANIMATION_H_
