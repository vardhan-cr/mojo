// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/threading/sequenced_worker_pool.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"

namespace mojo {

class URLResponseDiskCacheImpl : public URLResponseDiskCache {
 public:
  using FilePathPairCallback =
      base::Callback<void(const base::FilePath&, const base::FilePath&)>;

  URLResponseDiskCacheImpl(scoped_refptr<base::SequencedWorkerPool> worker_pool,
                           const std::string& remote_application_url,
                           InterfaceRequest<URLResponseDiskCache> request);
  ~URLResponseDiskCacheImpl() override;

 private:
  // URLResponseDiskCache
  void GetFile(mojo::URLResponsePtr response,
               const GetFileCallback& callback) override;
  void GetExtractedContent(
      mojo::URLResponsePtr response,
      const GetExtractedContentCallback& callback) override;

  // As |GetFile|, but uses FilePath instead of mojo arrays.
  void GetFileInternal(mojo::URLResponsePtr response,
                       const FilePathPairCallback& callback);

  // Internal implementation of |GetExtractedContent|. The parameters are:
  // |callback|: The callback to return values to the caller. It uses FilePath
  //             instead of mojo arrays.
  // |cache_dir|: The cache dir to return to the consumer.
  // |content|: The content of the body of the response.
  // |dir|: The base directory under which all data for the response is stored.
  void GetExtractedContentInternal(const FilePathPairCallback& callback,
                                   const base::FilePath& cache_dir,
                                   const base::FilePath& content,
                                   const base::FilePath& dir);

  scoped_refptr<base::SequencedWorkerPool> worker_pool_;
  base::FilePath base_directory_;
  StrongBinding<URLResponseDiskCache> binding_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheImpl);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
