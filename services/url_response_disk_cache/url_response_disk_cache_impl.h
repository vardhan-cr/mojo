// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
#define SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"

namespace mojo {

class URLResponseDiskCacheImpl : public URLResponseDiskCache {
 public:
  using FilePathPairCallback =
      base::Callback<void(const base::FilePath&, const base::FilePath&)>;

  // Cleares the cached content. The actual deletion will be performed using the
  // given task runner, but cache appears as cleared immediately after the
  // function returns.
  static void ClearCache(base::TaskRunner* task_runner);

  URLResponseDiskCacheImpl(base::TaskRunner* task_runner,
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
  // |base_dir|: The base directory for caching data associated to the response.
  // |extracted_dir|: The directory where the file content must be extracted. It
  //                  will be  returned to the consumer.
  // |content|: The content of the body of the response.
  // |cache_dir|: The directory the user can user to cache its own content.
  void GetExtractedContentInternal(const FilePathPairCallback& callback,
                                   const base::FilePath& base_dir,
                                   const base::FilePath& extracted_dir,
                                   const base::FilePath& content,
                                   const base::FilePath& cache_dir);

  base::TaskRunner* task_runner_;
  base::FilePath base_directory_;
  StrongBinding<URLResponseDiskCache> binding_;

  DISALLOW_COPY_AND_ASSIGN(URLResponseDiskCacheImpl);
};

}  // namespace mojo

#endif  // SERVICES_URL_RESPONSE_DISK_CACHE_URL_RESPONSE_DISK_CACHE_IMPL_H_
