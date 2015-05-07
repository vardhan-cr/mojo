// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_response_disk_cache_loader.h"

namespace shell {

URLResponseDiskCacheLoader::URLResponseDiskCacheLoader() {
}

URLResponseDiskCacheLoader::~URLResponseDiskCacheLoader() {
}

void URLResponseDiskCacheLoader::Load(
    const GURL& url,
    mojo::InterfaceRequest<mojo::Application> application_request) {
  DCHECK(application_request.is_pending());
  application_.reset(new mojo::ApplicationImpl(&url_response_disk_cache_,
                                               application_request.Pass()));
}

}  // namespace shell
