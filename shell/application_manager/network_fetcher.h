// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_
#define SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_

#include "shell/application_manager/fetcher.h"

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "mojo/services/network/public/interfaces/url_loader.mojom.h"
#include "mojo/services/url_response_disk_cache/public/interfaces/url_response_disk_cache.mojom.h"
#include "url/gurl.h"

namespace shell {

// Implements Fetcher for http[s] files.
class NetworkFetcher : public Fetcher {
 public:
  NetworkFetcher(bool disable_cache,
                 bool predictable_app_filenames,
                 const GURL& url,
                 mojo::URLResponseDiskCache* url_response_disk_cache,
                 mojo::NetworkService* network_service,
                 const FetchCallback& loader_callback);

  ~NetworkFetcher() override;

 private:
  // TODO(hansmuller): Revisit this when a real peek operation is available.
  static const MojoDeadline kPeekTimeout = MOJO_DEADLINE_INDEFINITE;

  const GURL& GetURL() const override;
  GURL GetRedirectURL() const override;

  mojo::URLResponsePtr AsURLResponse(base::TaskRunner* task_runner,
                                     uint32_t skip) override;

  static void RecordCacheToURLMapping(const base::FilePath& path,
                                      const GURL& url);

  // AppIds should be be both predictable and unique, but any hash would work.
  // Currently we use sha256 from crypto/secure_hash.h
  static bool ComputeAppId(const base::FilePath& path,
                           std::string* digest_string);

  static bool RenameToAppId(const GURL& url,
                            const base::FilePath& old_path,
                            base::FilePath* new_path);

  void OnFileRetrievedFromCache(
      base::Callback<void(const base::FilePath&, bool)> callback,
      mojo::Array<uint8_t> path_as_array,
      mojo::Array<uint8_t> cache_dir);

  void AsPath(
      base::TaskRunner* task_runner,
      base::Callback<void(const base::FilePath&, bool)> callback) override;

  std::string MimeType() override;

  bool HasMojoMagic() override;

  bool PeekFirstLine(std::string* line) override;

  void StartNetworkRequest(const GURL& url,
                           mojo::NetworkService* network_service);

  void OnLoadComplete(mojo::URLResponsePtr response);

  const bool disable_cache_;
  const bool predictable_app_filenames_;
  const GURL url_;
  mojo::URLResponseDiskCache* url_response_disk_cache_;
  mojo::URLLoaderPtr url_loader_;
  mojo::URLResponsePtr response_;
  base::FilePath path_;
  base::WeakPtrFactory<NetworkFetcher> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(NetworkFetcher);
};

}  // namespace shell

#endif  // SHELL_APPLICATION_MANAGER_NETWORK_FETCHER_H_
