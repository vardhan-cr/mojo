// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_BASE_CURSOR_CURSOR_H_
#define UI_BASE_CURSOR_CURSOR_H_

#include "build/build_config.h"
#include "ui/base/ui_base_export.h"

namespace gfx {
class Point;
class Size;
}

#if defined(OS_WIN)
typedef struct HINSTANCE__* HINSTANCE;
typedef struct HICON__* HICON;
typedef HICON HCURSOR;
#endif

namespace ui {

#if defined(OS_WIN)
typedef ::HCURSOR PlatformCursor;
#elif defined(USE_X11)
typedef unsigned long PlatformCursor;
#else
typedef void* PlatformCursor;
#endif

}  // namespace ui

#endif  // UI_BASE_CURSOR_CURSOR_H_
