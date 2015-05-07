// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/url_response_disk_cache/url_response_disk_cache_app.h"

#include "services/url_response_disk_cache/url_response_disk_cache_impl.h"

namespace mojo {

namespace {

const size_t kMaxBlockingPoolThreads = 3;

}  // namespace

URLResponseDiskCacheApp::URLResponseDiskCacheApp() {
}

URLResponseDiskCacheApp::~URLResponseDiskCacheApp() {
  if (worker_pool_)
    worker_pool_->Shutdown();
}

bool URLResponseDiskCacheApp::ConfigureIncomingConnection(
    ApplicationConnection* connection) {
  connection->AddService<URLResponseDiskCache>(this);
  return true;
}

void URLResponseDiskCacheApp::Create(
    ApplicationConnection* connection,
    InterfaceRequest<URLResponseDiskCache> request) {
  if (!worker_pool_) {
    worker_pool_ = new base::SequencedWorkerPool(kMaxBlockingPoolThreads,
                                                 "URLResponseDiskCachePool");
  }
  new URLResponseDiskCacheImpl(
      worker_pool_, connection->GetRemoteApplicationURL(), request.Pass());
}

}  // namespace mojo
