// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_JS_JS_APP_BRIDGE_MODULE_H_
#define SERVICES_JS_JS_APP_BRIDGE_MODULE_H_

#include "gin/gin_export.h"
#include "v8/include/v8.h"

namespace js {

class JSApp;

// The JavaScript "services/public/js/mojo" module depends on this
// built-in module. It provides the bridge between the JSApp class and
// JavaScript.

class AppBridge {
 public:
  static const char kModuleName[];
  static v8::Local<v8::Value> GetModule(JSApp* js_app, v8::Isolate* isolate);
};

}  // namespace js

#endif  // SERVICES_JS_JS_APP_BRIDGE_MODULE_H_
