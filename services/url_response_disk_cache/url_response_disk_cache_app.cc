// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/url_response_disk_cache/url_response_disk_cache_app.h"

#include "services/url_response_disk_cache/url_response_disk_cache_impl.h"

namespace mojo {

URLResponseDiskCacheApp::URLResponseDiskCacheApp(base::TaskRunner* task_runner)
    : task_runner_(task_runner) {
}

URLResponseDiskCacheApp::~URLResponseDiskCacheApp() {
}

bool URLResponseDiskCacheApp::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  connection->AddService<URLResponseDiskCache>(this);
  return true;
}

void URLResponseDiskCacheApp::Create(
    ApplicationConnection* connection,
    InterfaceRequest<URLResponseDiskCache> request) {
  new URLResponseDiskCacheImpl(
      task_runner_, connection->GetRemoteApplicationURL(), request.Pass());
}

}  // namespace mojo
