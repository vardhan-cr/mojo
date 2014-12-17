// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_WINDOW_MANAGER_CONSTANTS_H_
#define EXAMPLES_RECIPES_WINDOW_MANAGER_CONSTANTS_H_

namespace recipes {
namespace window_manager {

// kWindowTypeKey is used by the window manager to identify the type of
// window. It's one of kWindowTypeValueImmersive or kWindowTypeValueTransient.
extern const char kWindowTypeKey[];
extern const char kWindowTypeValueImmersive[];
extern const char kWindowTypeValueTransient[];

// If this key is present (value is ignored) events on any descendants are
// forwarded to the connection that created the view with the key.
extern const char kCaptureEventsKey[];

}  // window_manager
}  // recipes

#endif  //  EXAMPLES_RECIPES_WINDOW_MANAGER_CONSTANTS_H_
