// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_resolver.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "shell/filename_util.h"
#include "url/url_util.h"

namespace mojo {
namespace shell {

URLResolver::URLResolver() {
  // Needed to treat first component of mojo URLs as host, not path.
  url::AddStandardScheme("mojo");
}

URLResolver::~URLResolver() {
}

void URLResolver::AddURLMapping(const GURL& url, const GURL& resolved_url) {
  url_map_[url] = resolved_url;
}

GURL URLResolver::ApplyURLMappings(const GURL& url) const {
  GURL mapped_url(url);
  for (;;) {
    const auto& it = url_map_.find(mapped_url);
    if (it == url_map_.end())
      break;
    mapped_url = it->second;
  }
  return mapped_url;
}

void URLResolver::SetMojoBaseURL(const GURL& mojo_base_url) {
  DCHECK(mojo_base_url.is_valid());
  // Force a trailing slash on the base_url to simplify resolving
  // relative files and URLs below.
  mojo_base_url_ = AddTrailingSlashIfNeeded(mojo_base_url);
}

GURL URLResolver::ResolveMojoURL(const GURL& mojo_url) const {
  if (mojo_url.scheme() != "mojo") {
    // The mapping has produced some sort of non-mojo: URL - file:, http:, etc.
    return mojo_url;
  } else {
    // It's still a mojo: URL, use the default mapping scheme.
    std::string suffix = "";
    if (mojo_url.has_query()) {
      suffix = "?" + mojo_url.query();
    }
    std::string lib = mojo_url.host() + ".mojo" + suffix;
    return mojo_base_url_.Resolve(lib);
  }
}

}  // namespace shell
}  // namespace mojo
