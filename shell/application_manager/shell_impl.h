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
class ApplicationManager;

class ShellImpl : public Shell, public ErrorHandler {
 public:
  ShellImpl(ScopedMessagePipeHandle handle,
            ApplicationManager* manager,
            const GURL& requested_url,
            const GURL& url);

  ShellImpl(ShellPtr* ptr,
            ApplicationManager* manager,
            const GURL& requested_url,
            const GURL& url);

  ~ShellImpl() override;

  void ConnectToClient(const GURL& requestor_url,
                       InterfaceRequest<ServiceProvider> services,
                       ServiceProviderPtr exposed_services);

  Application* client() { return binding_.client(); }
  const GURL& url() const { return url_; }
  const GURL& requested_url() const { return requested_url_; }

 private:
  ShellImpl(ApplicationManager* manager,
            const GURL& requested_url,
            const GURL& url);

  // Shell implementation:
  void ConnectToApplication(const String& app_url,
                            InterfaceRequest<ServiceProvider> services,
                            ServiceProviderPtr exposed_services) override;

  // ErrorHandler implementation:
  void OnConnectionError() override;

  ApplicationManager* const manager_;
  const GURL requested_url_;
  const GURL url_;
  Binding<Shell> binding_;

  DISALLOW_COPY_AND_ASSIGN(ShellImpl);
};

}  // namespace mojo

#endif  // SHELL_APPLICATION_MANAGER_SHELL_IMPL_H_
