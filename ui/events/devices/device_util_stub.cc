// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/devices/device_util_linux.h"

namespace ui {

InputDeviceType GetInputDeviceTypeFromPath(const base::FilePath& path) {
  return InputDeviceType::INPUT_DEVICE_UNKNOWN;
}

}  // namespace
