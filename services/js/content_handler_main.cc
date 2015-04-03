// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/i18n/icu_util.h"
#include "gin/array_buffer.h"
#include "gin/public/isolate_holder.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/js/js_app.h"

namespace js {

class JsContentHandler : public mojo::ApplicationDelegate,
                         public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  JsContentHandler() : content_handler_factory_(this) {}

 private:
  // Overridden from mojo::ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    static const char v8Flags[] = "--harmony-classes";
    v8::V8::SetFlagsFromString(v8Flags, sizeof(v8Flags) - 1);
    base::i18n::InitializeICU();
    gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                   gin::ArrayBufferAllocator::SharedInstance());
  }

  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override {
    return make_scoped_ptr(
        new JSApp(application_request.Pass(), response.Pass()));
  }

  mojo::ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(JsContentHandler);
};

}  // namespace js

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new js::JsContentHandler);
  return runner.Run(application_request);
}
