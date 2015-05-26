// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "shell/authenticating_url_loader_loader.h"

namespace shell {

AuthenticatingURLLoaderLoader::AuthenticatingURLLoaderLoader() {
}

AuthenticatingURLLoaderLoader::~AuthenticatingURLLoaderLoader() {
}

void AuthenticatingURLLoaderLoader::Load(
    const GURL& url,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  DCHECK(application_request.is_pending());
  application_.reset(new mojo::ApplicationImpl(&authenticating_url_loader_,
                                               application_request.Pass()));
}

}  // namespace shell
