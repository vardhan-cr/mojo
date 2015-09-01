// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_

#include "base/macros.h"
#include "base/task_runner.h"
#include "base/threading/sequenced_worker_pool.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"

namespace mojo {

class URLResponseDiskCacheApp : public ApplicationDelegate,
                                public InterfaceFactory<URLResponseDiskCache> {
 public:
  URLResponseDiskCacheApp(base::TaskRunner* task_runner);
  ~URLResponseDiskCacheApp() override;

 private:
  // ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override;

  // InterfaceFactory<URLResponseDiskCache>:
  void Create(ApplicationConnection* connection,
              InterfaceRequest<URLResponseDiskCache> request) override;

  base::TaskRunner* task_runner_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheApp);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_APP_H_
