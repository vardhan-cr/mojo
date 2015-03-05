// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
#define SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/error_handler.h"
#include "mojo/public/interfaces/application/application.mojom.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {

class ApplicationManager;

class ShellImpl : public Shell, public ErrorHandler {
 public:
  ShellImpl(ApplicationPtr application,
            ApplicationManager* manager,
            // The original URL that was first requested, before any resolution.
            const GURL& original_url,
            const GURL& resolved_url);

  ~ShellImpl() override;

  void InitializeApplication(Array<String> args);

  void ConnectToClient(const GURL& requested_url,
                       const GURL& requestor_url,
                       InterfaceRequest<ServiceProvider> services,
                       ServiceProviderPtr exposed_services);

  Application* application() { return application_.get(); }
  const GURL& url() const { return url_; }
  const GURL& requested_url() const { return requested_url_; }

 private:
  // Shell implementation:
  void ConnectToApplication(const String& app_url,
                            InterfaceRequest<ServiceProvider> services,
                            ServiceProviderPtr exposed_services) override;

  // ErrorHandler implementation:
  void OnConnectionError() override;

  ApplicationManager* const manager_;
  const GURL requested_url_;
  const GURL url_;
  ApplicationPtr application_;
  Binding<Shell> binding_;

  DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
