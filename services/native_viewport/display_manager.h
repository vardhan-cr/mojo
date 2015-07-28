// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_DISPLAY_MANAGER_H_
#define SERVICES_NATIVE_VIEWPORT_DISPLAY_MANAGER_H_

#include <vector>
#include "base/callback.h"
#include "ui/display/types/native_display_delegate.h"
#include "ui/display/types/native_display_observer.h"
#include "ui/gfx/geometry/rect.h"

namespace native_viewport {

class DisplayManager : public ui::NativeDisplayObserver {
 public:
  DisplayManager(const base::Closure& quit_closure);
  ~DisplayManager() override;

  void Quit();

 private:
  void OnDisplaysAquired(const std::vector<ui::DisplaySnapshot*>& displays);
  void OnDisplayConfigured(const gfx::Rect& bounds, bool success);

  // ui::NativeDisplayObserver
  void OnConfigurationChanged() override;

  scoped_ptr<ui::NativeDisplayDelegate> delegate_;
  base::Closure quit_closure_;

  // Flags used to keep track of the current state of display configuration.
  //
  // True if configuring the displays. In this case a new display configuration
  // isn't started.
  bool is_configuring_ = false;

  // If |is_configuring_| is true and another display configuration event
  // happens, the event is deferred. This is set to true and a display
  // configuration will be scheduled after the current one finishes.
  bool should_configure_ = false;

  DISALLOW_COPY_AND_ASSIGN(DisplayManager);
};

} // namespace

#endif

