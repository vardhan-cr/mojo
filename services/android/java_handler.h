// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_ANDROID_JAVA_HANDLER_H_
#define SERVICES_ANDROID_JAVA_HANDLER_H_

#include <jni.h>

#include "mojo/application/content_handler_factory.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace base {
class FilePath;
}

namespace services {
namespace android {

class JavaHandler : public mojo::ApplicationDelegate,
                    public mojo::ContentHandlerFactory::Delegate {
 public:
  JavaHandler();
  ~JavaHandler();

 private:
  // ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // ContentHandlerFactory::Delegate:
  void RunApplication(
      mojo::InterfaceRequest<mojo::Application> application_request,
      mojo::URLResponsePtr response) override;

  mojo::ContentHandlerFactory content_handler_factory_;
  MOJO_DISALLOW_COPY_AND_ASSIGN(JavaHandler);
};

}  // namespace android
}  // namespace services

#endif  // SERVICES_ANDROID_JAVA_HANDLER_H_
