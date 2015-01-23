// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/shell_impl.h"

#include "mojo/common/common_type_converters.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"
#include "shell/application_manager/application_manager.h"

namespace mojo {

ShellImpl::ShellImpl(InterfaceRequest<Shell> shell_request,
                     ApplicationManager* manager,
                     const GURL& requested_url,
                     const GURL& url)
    : manager_(manager),
      requested_url_(requested_url),
      url_(url),
      binding_(this, shell_request.Pass()) {
  binding_.set_error_handler(this);
}

ShellImpl::~ShellImpl() {
}

void ShellImpl::ConnectToClient(const GURL& requestor_url,
                                InterfaceRequest<ServiceProvider> services,
                                ServiceProviderPtr exposed_services) {
  client()->AcceptConnection(String::From(requestor_url), services.Pass(),
                             exposed_services.Pass());
}

// Shell implementation:
void ShellImpl::ConnectToApplication(const String& app_url,
                                     InterfaceRequest<ServiceProvider> services,
                                     ServiceProviderPtr exposed_services) {
  GURL app_gurl(app_url);
  if (!app_gurl.is_valid()) {
    LOG(ERROR) << "Error: invalid URL: " << app_url;
    return;
  }
  manager_->ConnectToApplication(app_gurl, url_, services.Pass(),
                                 exposed_services.Pass());
}

void ShellImpl::OnConnectionError() {
  manager_->OnShellImplError(this);
}

}  // namespace mojo
