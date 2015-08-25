// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/network_fetcher.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/message_loop/message_loop.h"
#include "base/process/process.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "crypto/secure_hash.h"
#include "crypto/sha2.h"
#include "mojo/converters/url/url_type_converters.h"
#include "mojo/data_pipe_utils/data_pipe_utils.h"
#include "shell/application_manager/data_pipe_peek.h"

namespace shell {

namespace {
#if defined(OS_LINUX)
char kArchitecture[] = "linux-x64";
#elif defined(OS_ANDROID)
char kArchitecture[] = "android-arm";
#else
#error "Unsupported."
#endif
};

NetworkFetcher::NetworkFetcher(
    bool disable_cache,
    const GURL& url,
    mojo::URLResponseDiskCache* url_response_disk_cache,
    mojo::NetworkService* network_service,
    const FetchCallback& loader_callback)
    : Fetcher(loader_callback),
      disable_cache_(disable_cache),
      url_(url),
      url_response_disk_cache_(url_response_disk_cache),
      network_service_(network_service),
      weak_ptr_factory_(this) {
  StartNetworkRequest(FROM_NETWORK);
}

NetworkFetcher::~NetworkFetcher() {
}

const GURL& NetworkFetcher::GetURL() const {
  return url_;
}

GURL NetworkFetcher::GetRedirectURL() const {
  if (!response_)
    return GURL::EmptyGURL();

  if (response_->redirect_url.is_null())
    return GURL::EmptyGURL();

  return GURL(response_->redirect_url);
}

mojo::URLResponsePtr NetworkFetcher::AsURLResponse(
    base::TaskRunner* task_runner,
    uint32_t skip) {
  if (skip != 0) {
    MojoResult result = ReadDataRaw(
        response_->body.get(), nullptr, &skip,
        MOJO_READ_DATA_FLAG_ALL_OR_NONE | MOJO_READ_DATA_FLAG_DISCARD);
    DCHECK_EQ(result, MOJO_RESULT_OK);
  }
  return response_.Pass();
}

void NetworkFetcher::RecordCacheToURLMapping(const base::FilePath& path,
                                             const GURL& url) {
  // This is used to extract symbols on android.
  // TODO(eseidel): All users of this log should move to using the map file.
  LOG(INFO) << "Caching mojo app " << url << " at " << path.value();

  base::FilePath temp_dir;
  base::GetTempDir(&temp_dir);
  base::ProcessId pid = base::Process::Current().Pid();
  std::string map_name = base::StringPrintf("mojo_shell.%d.maps", pid);
  base::FilePath map_path = temp_dir.Append(map_name);

  // TODO(eseidel): Paths or URLs with spaces will need quoting.
  std::string map_entry =
      base::StringPrintf("%s %s\n", path.value().c_str(), url.spec().c_str());
  // TODO(eseidel): AppendToFile is missing O_CREAT, crbug.com/450696
  if (!PathExists(map_path))
    base::WriteFile(map_path, map_entry.data(), map_entry.length());
  else
    base::AppendToFile(map_path, map_entry.data(), map_entry.length());
}

void NetworkFetcher::OnFileRetrievedFromCache(
    base::Callback<void(const base::FilePath&, bool)> callback,
    mojo::Array<uint8_t> path_as_array,
    mojo::Array<uint8_t> cache_dir) {
  bool success = !path_as_array.is_null();
  if (success) {
    path_ = base::FilePath(std::string(
        reinterpret_cast<char*>(&path_as_array.front()), path_as_array.size()));
    RecordCacheToURLMapping(path_, url_);
  }

  base::MessageLoop::current()->PostTask(FROM_HERE,
                                         base::Bind(callback, path_, success));
}

void NetworkFetcher::AsPath(
    base::TaskRunner* task_runner,
    base::Callback<void(const base::FilePath&, bool)> callback) {
  // This should only called once, when we have a response.
  DCHECK(response_.get());

  url_response_disk_cache_->GetFile(
      response_.Pass(), base::Bind(&NetworkFetcher::OnFileRetrievedFromCache,
                                   weak_ptr_factory_.GetWeakPtr(), callback));
}

std::string NetworkFetcher::MimeType() {
  return response_->mime_type;
}

bool NetworkFetcher::HasMojoMagic() {
  std::string magic;
  return BlockingPeekNBytes(response_->body.get(), &magic, strlen(kMojoMagic),
                            kPeekTimeout) &&
         magic == kMojoMagic;
}

bool NetworkFetcher::PeekFirstLine(std::string* line) {
  return BlockingPeekLine(response_->body.get(), line, kMaxShebangLength,
                          kPeekTimeout);
}

void NetworkFetcher::StartNetworkRequest(RequestType request_type) {
  TRACE_EVENT_ASYNC_BEGIN1("mojo_shell", "NetworkFetcher::NetworkRequest", this,
                           "url", url_.spec());
  mojo::URLRequestPtr request(mojo::URLRequest::New());
  request->url = mojo::String::From(url_);
  request->auto_follow_redirects = false;
  if (disable_cache_)
    request->cache_mode = mojo::URLRequest::CACHE_MODE_BYPASS_CACHE;
  auto header = mojo::HttpHeader::New();
  header->name = "X-Architecture";
  header->value = kArchitecture;
  mojo::Array<mojo::HttpHeaderPtr> headers;
  headers.push_back(header.Pass());
  request->headers = headers.Pass();

  network_service_->CreateURLLoader(GetProxy(&url_loader_));
  url_loader_->Start(request.Pass(),
                     base::Bind(&NetworkFetcher::OnLoadComplete,
                                weak_ptr_factory_.GetWeakPtr(), request_type));
}

void NetworkFetcher::OnLoadComplete(RequestType request_type,
                                    mojo::URLResponsePtr response) {
  TRACE_EVENT_ASYNC_END0("mojo_shell", "NetworkFetcher::NetworkRequest", this);
  scoped_ptr<Fetcher> owner(this);
  if (response->error) {
    LOG(ERROR) << "Error (" << response->error->code << ": "
               << response->error->description << ") while fetching "
               << response->url;
    if (request_type == FROM_NETWORK) {
      StartNetworkRequest(FROM_CACHE);
    } else {
      loader_callback_.Run(nullptr);
    }
    return;
  }

  if (response->status_code >= 400 && response->status_code < 600) {
    LOG(ERROR) << "Error (" << response->status_code << ": "
               << response->status_line << "): "
               << "while fetching " << response->url;
    loader_callback_.Run(nullptr);
    return;
  }

  response_ = response.Pass();
  loader_callback_.Run(owner.Pass());
}

}  // namespace shell
