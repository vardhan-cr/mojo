// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_
#define MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_

#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/gpu/public/interfaces/gpu.mojom.h"
#include "mojo/services/keyboard/public/interfaces/keyboard.mojom.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "services/gles2/gpu_impl.h"
#include "shell/application_manager/application_loader.h"

namespace gles2 {
class GpuState;
}

namespace mojo {

class ApplicationImpl;

namespace shell {

class NativeViewportApplicationLoader : public ApplicationLoader,
                                        public ApplicationDelegate,
                                        public InterfaceFactory<Keyboard>,
                                        public InterfaceFactory<NativeViewport>,
                                        public InterfaceFactory<Gpu> {
 public:
  NativeViewportApplicationLoader();
  ~NativeViewportApplicationLoader();

 private:
  // ApplicationLoader implementation.
  void Load(const GURL& url,
            InterfaceRequest<Application> application_request) override;

  // ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // InterfaceFactory<NativeViewport> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<NativeViewport> request) override;

  // InterfaceFactory<Gpu> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<Gpu> request) override;

  // InterfaceFactory<Keyboard> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<Keyboard> request) override;

  scoped_refptr<gles2::GpuState> gpu_state_;
  scoped_ptr<ApplicationImpl> app_;

  DISALLOW_COPY_AND_ASSIGN(NativeViewportApplicationLoader);
};

}  // namespace shell
}  // namespace mojo

#endif  // MOJO_SHELL_ANDROID_NATIVE_VIEWPORT_APPLICATION_LOADER_H_
