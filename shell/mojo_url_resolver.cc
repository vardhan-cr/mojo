// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/mojo_url_resolver.h"

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "shell/filename_util.h"
#include "url/url_util.h"

namespace mojo {
namespace shell {

MojoURLResolver::MojoURLResolver() {
  // Needed to treat first component of mojo URLs as host, not path.
  url::AddStandardScheme("mojo");
}

MojoURLResolver::~MojoURLResolver() {
}

void MojoURLResolver::SetBaseURL(const GURL& base_url) {
  DCHECK(base_url.is_valid());
  // Force a trailing slash on the base_url to simplify resolving
  // relative files and URLs below.
  base_url_ = AddTrailingSlashIfNeeded(base_url);
}

void MojoURLResolver::AddCustomMapping(const GURL& mojo_url,
                                       const GURL& resolved_url) {
  url_map_[mojo_url] = resolved_url;
}

GURL MojoURLResolver::Resolve(const GURL& mojo_url) const {
  const GURL mapped_url(ApplyCustomMappings(mojo_url));

  if (mapped_url.scheme() != "mojo") {
    // The mapping has produced some sort of non-mojo: URL - file:, http:, etc.
    return mapped_url;
  } else {
    // It's still a mojo: URL, use the default mapping scheme.
    std::string lib = mapped_url.host() + ".mojo";
    return base_url_.Resolve(lib);
  }
}

GURL MojoURLResolver::ApplyCustomMappings(const GURL& url) const {
  GURL mapped_url(url);
  for (;;) {
    std::map<GURL, GURL>::const_iterator it = url_map_.find(mapped_url);
    if (it == url_map_.end())
      break;
    mapped_url = it->second;
  }
  return mapped_url;
}

}  // namespace shell
}  // namespace mojo
