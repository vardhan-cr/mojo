// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/native_viewport_application_loader.h"

#include "mojo/public/cpp/application/application_impl.h"
#include "services/native_viewport/native_viewport_impl.h"

namespace mojo {
namespace shell {

NativeViewportApplicationLoader::NativeViewportApplicationLoader()
    : gpu_state_(new gles2::GpuImpl::State) {
}

NativeViewportApplicationLoader::~NativeViewportApplicationLoader() {
}

void NativeViewportApplicationLoader::Load(ApplicationManager* manager,
                                           const GURL& url,
                                           ScopedMessagePipeHandle shell_handle,
                                           LoadCallback callback) {
  DCHECK(shell_handle.is_valid());
  app_.reset(new ApplicationImpl(this, shell_handle.Pass()));
}

void NativeViewportApplicationLoader::OnApplicationError(
    ApplicationManager* manager,
    const GURL& url) {
}

bool NativeViewportApplicationLoader::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<NativeViewport>(this);
  connection->AddService<Gpu>(this);
  return true;
}

void NativeViewportApplicationLoader::Create(
    ApplicationConnection* connection,
    InterfaceRequest<NativeViewport> request) {
  BindToRequest(new native_viewport::NativeViewportImpl(app_.get(), false),
                &request);
}

void NativeViewportApplicationLoader::Create(
    ApplicationConnection* connection,
    InterfaceRequest<Gpu> request) {
  new gles2::GpuImpl(request.Pass(), gpu_state_);
}

}  // namespace shell
}  // namespace mojo
