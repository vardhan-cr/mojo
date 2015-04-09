// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/shell_impl.h"

#include "mojo/common/common_type_converters.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"
#include "shell/application_manager/application_manager.h"

namespace shell {

ShellImpl::ShellImpl(mojo::ApplicationPtr application,
                     ApplicationManager* manager,
                     const Identity& identity,
                     const base::Closure& on_application_end)
    : manager_(manager),
      identity_(identity),
      on_application_end_(on_application_end),
      application_(application.Pass()),
      binding_(this) {
  binding_.set_error_handler(this);
}

ShellImpl::~ShellImpl() {
}

void ShellImpl::InitializeApplication(mojo::Array<mojo::String> args) {
  mojo::ShellPtr shell;
  binding_.Bind(mojo::GetProxy(&shell));
  application_->Initialize(shell.Pass(), args.Pass(), identity_.url.spec());
}

void ShellImpl::ConnectToClient(
    const GURL& requested_url,
    const GURL& requestor_url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  application_->AcceptConnection(mojo::String::From(requestor_url),
                                 services.Pass(), exposed_services.Pass(),
                                 requested_url.spec());
}

// Shell implementation:
void ShellImpl::ConnectToApplication(
    const mojo::String& app_url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  GURL app_gurl(app_url);
  if (!app_gurl.is_valid()) {
    LOG(ERROR) << "Error: invalid URL: " << app_url;
    return;
  }
  manager_->ConnectToApplication(app_gurl, identity_.url, services.Pass(),
                                 exposed_services.Pass(), base::Closure());
}

void ShellImpl::OnConnectionError() {
  manager_->OnShellImplError(this);
}

}  // namespace shell
