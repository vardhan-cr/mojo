// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/location.h"
#include "base/thread_task_runner_handle.h"
#include "services/native_viewport/display_manager.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/ozone/public/ozone_platform.h"

namespace native_viewport {

DisplayManager::DisplayManager(const base::Closure& quit_closure)
    : delegate_(
          ui::OzonePlatform::GetInstance()->CreateNativeDisplayDelegate()),
      quit_closure_(quit_closure) {
  
  LOG(INFO) << "DisplayManager() " << delegate_;
  
  delegate_->AddObserver(this);
  delegate_->Initialize();
  OnConfigurationChanged();
}

DisplayManager::~DisplayManager() {
  if (delegate_)
    delegate_->RemoveObserver(this);
}

void DisplayManager::Quit() {
  quit_closure_.Run();
}

void DisplayManager::OnConfigurationChanged() {
  LOG(INFO) << "DisplayManager::OnConfigurationChanged";
  if (is_configuring_) {
    should_configure_ = true;
    return;
  }

  is_configuring_ = true;
  delegate_->GrabServer();
  delegate_->GetDisplays(
      base::Bind(&DisplayManager::OnDisplaysAquired, base::Unretained(this)));
}

void DisplayManager::OnDisplaysAquired(
    const std::vector<ui::DisplaySnapshot*>& displays) {
  LOG(INFO) << "DisplayManager::OnDisplaysAquired";

  gfx::Point origin;
  for (auto display : displays) {
    if (!display->native_mode()) {
      LOG(ERROR) << "Display " << display->display_id()
                 << " doesn't have a native mode";
      continue;
    }

    delegate_->Configure(
        *display, display->native_mode(), origin,
        base::Bind(&DisplayManager::OnDisplayConfigured, base::Unretained(this),
                   gfx::Rect(origin, display->native_mode()->size())));
    origin.Offset(display->native_mode()->size().width(), 0);
  }
  delegate_->UngrabServer();
  is_configuring_ = false;

  if (should_configure_) {
    should_configure_ = false;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&DisplayManager::OnConfigurationChanged,
                              base::Unretained(this)));
  }
}

void DisplayManager::OnDisplayConfigured(const gfx::Rect& bounds, bool success) {
  LOG(INFO) << "OnDisplayConfigured " << success;
}

} // namespace
