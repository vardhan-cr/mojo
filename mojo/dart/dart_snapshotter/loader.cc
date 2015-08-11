// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/dart/dart_snapshotter/loader.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "tonic/dart_converter.h"
#include "tonic/dart_error.h"

namespace {

std::string Fetch(const std::string& url) {
  base::FilePath path(url);
  std::string source;
  CHECK(base::ReadFileToString(path, &source)) << url;
  return source;
}

base::FilePath SimplifyPath(const base::FilePath& path) {
  std::vector<base::FilePath::StringType> components;
  path.GetComponents(&components);
  base::FilePath result;
  if (path.IsAbsolute()) {
    base::FilePath root("/");
    result = root;
  }
  for (const auto& component : components) {
    base::FilePath c(component);
    if (c.IsAbsolute()) continue;
    if (component == base::FilePath::kCurrentDirectory)
      continue;
    if (component == base::FilePath::kParentDirectory)
      result = result.DirName();
    else
      result = result.Append(component);
  }
  return result;
}

class Loader {
 public:
  Loader(const base::FilePath& package_root);

  std::string CanonicalizePackageURL(std::string url);
  Dart_Handle CanonicalizeURL(Dart_Handle library, Dart_Handle url);
  Dart_Handle Import(Dart_Handle url);
  Dart_Handle Source(Dart_Handle library, Dart_Handle url);

 private:
  base::FilePath package_root_;

  DISALLOW_COPY_AND_ASSIGN(Loader);
};

Loader::Loader(const base::FilePath& package_root)
    : package_root_(package_root) {
}

std::string Loader::CanonicalizePackageURL(std::string url) {
  DCHECK(StartsWithASCII(url, "package:", true));
  ReplaceFirstSubstringAfterOffset(&url, 0, "package:", "");
  return package_root_.Append(url).AsUTF8Unsafe();
}

Dart_Handle Loader::CanonicalizeURL(Dart_Handle library, Dart_Handle url) {
  std::string string = tonic::StdStringFromDart(url);
  if (StartsWithASCII(string, "dart:", true))
    return url;
  if (StartsWithASCII(string, "package:", true))
    return tonic::StdStringToDart(CanonicalizePackageURL(string));
  base::FilePath base_path(tonic::StdStringFromDart(Dart_LibraryUrl(library)));
  base::FilePath resolved_path = base_path.DirName().Append(string);
  base::FilePath normalized_path = SimplifyPath(resolved_path);
  return tonic::StdStringToDart(normalized_path.AsUTF8Unsafe());
}

Dart_Handle Loader::Import(Dart_Handle url) {
  Dart_Handle source =
      tonic::StdStringToDart(Fetch(tonic::StdStringFromDart(url)));
  Dart_Handle result = Dart_LoadLibrary(url, source, 0, 0);
  tonic::LogIfError(result);
  return result;
}

Dart_Handle Loader::Source(Dart_Handle library, Dart_Handle url) {
  Dart_Handle source =
      tonic::StdStringToDart(Fetch(tonic::StdStringFromDart(url)));
  Dart_Handle result = Dart_LoadSource(library, url, source, 0, 0);
  tonic::LogIfError(result);
  return result;
}

Loader* g_loader = nullptr;

Loader& GetLoader() {
  CHECK(g_loader != nullptr);
  return *g_loader;
}

}  // namespace

void InitLoader(const base::FilePath& package_root) {
  CHECK(g_loader == nullptr);
  g_loader = new Loader(package_root);
}

Dart_Handle HandleLibraryTag(Dart_LibraryTag tag,
                             Dart_Handle library,
                             Dart_Handle url) {
  CHECK(Dart_IsLibrary(library));
  CHECK(Dart_IsString(url));

  if (tag == Dart_kCanonicalizeUrl)
    return GetLoader().CanonicalizeURL(library, url);

  if (tag == Dart_kImportTag)
    return GetLoader().Import(url);

  if (tag == Dart_kSourceTag)
    return GetLoader().Source(library, url);

  return Dart_NewApiError("Unknown library tag.");
}

void LoadScript(const std::string& url) {
  tonic::LogIfError(Dart_LoadScript(
      tonic::StdStringToDart(url), tonic::StdStringToDart(Fetch(url)), 0, 0));
}
