// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/js/js_app_shell.h"

#include "gin/object_template_builder.h"
#include "services/js/js_app.h"

namespace js {

gin::WrapperInfo JSAppShell::kWrapperInfo = {gin::kEmbedderNativeGin};

gin::Handle<JSAppShell> JSAppShell::Create(v8::Isolate* isolate,
                                           JSApp* js_app) {
  return gin::CreateHandle(isolate, new JSAppShell(js_app));
}

JSAppShell::JSAppShell(JSApp* js_app) : js_app_(js_app) {
}

JSAppShell::~JSAppShell() {
}

gin::ObjectTemplateBuilder JSAppShell::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<JSAppShell>::GetObjectTemplateBuilder(isolate)
      .SetMethod("connectToApplication", &JSAppShell::ConnectToApplication);
}

void JSAppShell::ConnectToApplication(const std::string& application_url,
                                      v8::Handle<v8::Value> service_provider) {
  // TODO(hansmuller): Validate arguments.
  js_app_->ConnectToApplication(application_url, service_provider);
}

}  // namespace js
