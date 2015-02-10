// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/url_resolver.h"

#include "base/logging.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace shell {
namespace test {
namespace {

typedef testing::Test URLResolverTest;

TEST_F(URLResolverTest, MojoURLsFallThrough) {
  URLResolver resolver;
  resolver.AddURLMapping(GURL("mojo:test"), GURL("mojo:foo"));
  const GURL base_url("file:/base");
  resolver.SetMojoBaseURL(base_url);
  GURL mapped_url = resolver.ApplyURLMappings(GURL("mojo:test"));
  std::string resolved(resolver.ResolveMojoURL(mapped_url).spec());
  // Resolved must start with |base_url|.
  EXPECT_EQ(base_url.spec(), resolved.substr(0, base_url.spec().size()));
  // And must contain foo.
  EXPECT_NE(std::string::npos, resolved.find("foo"));
}

}  // namespace
}  // namespace test
}  // namespace shell
}  // namespace mojo
