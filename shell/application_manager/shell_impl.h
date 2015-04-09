// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
#define SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_

#include "base/callback.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "shell/application_manager/identity.h"
#include "url/gurl.h"

namespace shell {

class ApplicationManager;

class ShellImpl : public mojo::Shell, public mojo::ErrorHandler {
 public:
  ShellImpl(mojo::ApplicationPtr application,
            ApplicationManager* manager,
            const Identity& resolved_identity,
            const base::Closure& on_application_end);

  ~ShellImpl() override;

  void InitializeApplication(mojo::Array<mojo::String> args);

  void ConnectToClient(const GURL& requested_url,
                       const GURL& requestor_url,
                       mojo::InterfaceRequest<mojo::ServiceProvider> services,
                       mojo::ServiceProviderPtr exposed_services);

  mojo::Application* application() { return application_.get(); }
  const Identity& identity() const { return identity_; }
  base::Closure on_application_end() const { return on_application_end_; }

 private:
  // mojo::Shell implementation:
  void ConnectToApplication(
      const mojo::String& app_url,
      mojo::InterfaceRequest<mojo::ServiceProvider> services,
      mojo::ServiceProviderPtr exposed_services) override;

  // mojo::ErrorHandler implementation:
  void OnConnectionError() override;

  ApplicationManager* const manager_;
  const Identity identity_;
  base::Closure on_application_end_;
  mojo::ApplicationPtr application_;
  mojo::Binding<mojo::Shell> binding_;

  DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
