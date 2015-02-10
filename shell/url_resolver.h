// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_URL_RESOLVER_H_
#define SHELL_URL_RESOLVER_H_

#include <map>
#include <set>

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {

// This class supports the mapping of URLs to other URLs.
// It's commonly used with mojo: URL, to provide a physical location (i.e.
// file: or https:) but works with any URL.
// By default, "mojo:" URLs resolve to a file location, with ".mojo" appended,
// but that resolution can be customized via the AddCustomMapping method.
class URLResolver {
 public:
  URLResolver();
  ~URLResolver();

  // Add a custom mapping for a particular URL. If |resolved_url| is
  // itself a mojo url normal resolution rules apply.
  void AddURLMapping(const GURL& url, const GURL& resolved_url);

  // Applies all custom mappings for |url|, returning the last non-mapped url.
  // For example, if 'a' maps to 'b' and 'b' maps to 'c' calling this with 'a'
  // returns 'c'.
  GURL ApplyURLMappings(const GURL& url) const;

  // If specified, then "mojo:" URLs will be resolved relative to this
  // URL. That is, the portion after the colon will be appeneded to
  // |mojo_base_url| with .mojo appended.
  void SetMojoBaseURL(const GURL& mojo_base_url);

  // Resolve the given "mojo:" URL to the URL that should be used to fetch the
  // code for the corresponding Mojo App.
  GURL ResolveMojoURL(const GURL& mojo_url) const;

 private:
  std::map<GURL, GURL> url_map_;
  GURL mojo_base_url_;

  DISALLOW_COPY_AND_ASSIGN(URLResolver);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_URL_RESOLVER_H_
