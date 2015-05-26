// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_AUTHENTICATING_URL_LOADER_LOADER_H_
#define SHELL_AUTHENTICATING_URL_LOADER_LOADER_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/authenticating_url_loader/authenticating_url_loader_app.h"
#include "shell/application_manager/application_loader.h"

namespace shell {

class AuthenticatingURLLoaderLoader : public ApplicationLoader {
 public:
  AuthenticatingURLLoaderLoader();
  ~AuthenticatingURLLoaderLoader() override;

 private:
  // ApplicationLoader overrides:
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override;

  mojo::AuthenticatingURLLoaderApp authenticating_url_loader_;
  scoped_ptr<mojo::ApplicationImpl> application_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatingURLLoaderLoader);
};

}  // namespace shell

#endif  // SHELL_AUTHENTICATING_URL_LOADER_LOADER_H_
