// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/application_manager/shell_impl.h"

#include "mojo/application_manager/application_manager.h"
#include "mojo/common/common_type_converters.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace mojo {

ShellImpl::ShellImpl(ScopedMessagePipeHandle handle,
                     ApplicationManager* manager,
                     const GURL& requested_url,
                     const GURL& url)
    : ShellImpl(manager, requested_url, url) {
  binding_.Bind(handle.Pass());
}

ShellImpl::ShellImpl(ShellPtr* ptr,
                     ApplicationManager* manager,
                     const GURL& requested_url,
                     const GURL& url)
    : ShellImpl(manager, requested_url, url) {
  binding_.Bind(ptr);
}

ShellImpl::~ShellImpl() {
}

void ShellImpl::ConnectToClient(const GURL& requestor_url,
                                ServiceProviderPtr service_provider) {
  client()->AcceptConnection(String::From(requestor_url),
                             service_provider.Pass());
}

ShellImpl::ShellImpl(ApplicationManager* manager,
                     const GURL& requested_url,
                     const GURL& url)
    : manager_(manager),
      requested_url_(requested_url),
      url_(url),
      binding_(this) {
  binding_.set_error_handler(this);
}

// Shell implementation:
void ShellImpl::ConnectToApplication(
    const String& app_url,
    InterfaceRequest<ServiceProvider> in_service_provider) {
  ServiceProviderPtr out_service_provider;
  out_service_provider.Bind(in_service_provider.PassMessagePipe());
  GURL app_gurl(app_url);
  if (!app_gurl.is_valid()) {
    LOG(ERROR) << "Error: invalid URL: " << app_url;
    return;
  }
  manager_->ConnectToApplication(app_gurl, url_, out_service_provider.Pass());
}

void ShellImpl::OnConnectionError() {
  manager_->OnShellImplError(this);
}

}  // namespace mojo
