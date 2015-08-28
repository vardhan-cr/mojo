// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/native_viewport/ozone/app_delegate_ozone.h"
#include "ui/ozone/public/ipc_init_helper_mojo.h"
#include "ui/ozone/public/ozone_platform.h"

namespace native_viewport {

void NativeViewportOzoneAppDelegate::Initialize(
    mojo::ApplicationImpl* application) {
  NativeViewportAppDelegate::Initialize(application);

  ui::OzonePlatform::InitializeForUI();

  auto ipc_init_helper = static_cast<ui::IpcInitHelperMojo*>(
      ui::OzonePlatform::GetInstance()->GetIpcInitHelperOzone());

  ipc_init_helper->HostInitialize(application);
  ipc_init_helper->GpuInitialize(application);

  display_manager_.reset(new DisplayManager());
}

bool NativeViewportOzoneAppDelegate::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  if (!NativeViewportAppDelegate::ConfigureIncomingConnection(connection))
    return false;

  auto ipc_init_helper = static_cast<ui::IpcInitHelperMojo*>(
      ui::OzonePlatform::GetInstance()->GetIpcInitHelperOzone());

  ipc_init_helper->HostConfigureIncomingConnection(connection);
  ipc_init_helper->GpuConfigureIncomingConnection(connection);

  return true;
}

}  // namespace native_viewport
