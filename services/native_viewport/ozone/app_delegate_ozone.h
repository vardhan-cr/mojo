// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NATIVE_VIEWPORT_APP_DELEGATE_OZONE_H_
#define SERVICES_NATIVE_VIEWPORT_APP_DELEGATE_OZONE_H_

#include "services/native_viewport/app_delegate.h"
#include "services/native_viewport/ozone/display_manager.h"

namespace native_viewport {

class NativeViewportOzoneAppDelegate : public NativeViewportAppDelegate {
 public:
  using NativeViewportAppDelegate::Create;

  void Initialize(mojo::ApplicationImpl* application) override;
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override;

 private:
  std::unique_ptr<DisplayManager> display_manager_;
};

}  // namespace native_viewport

#endif
