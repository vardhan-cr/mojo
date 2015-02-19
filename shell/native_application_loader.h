// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_NATIVE_APPLICATION_LOADER_H_
#define SHELL_NATIVE_APPLICATION_LOADER_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/network/public/interfaces/network_service.mojom.h"
#include "shell/application_manager/application_loader.h"
#include "shell/dynamic_service_runner.h"
#include "url/gurl.h"

namespace mojo {
namespace shell {

class Context;
class DynamicServiceRunnerFactory;
class DynamicServiceRunner;

// Loads applications that might be either DSOs or content applications.
// TODO(aa): Tear this apart into:
// - fetching urls (probably two sublcasses for http/file)
// - detecting type of response (dso, content, nacl (future))
// - executing response (either via DynamicServiceRunner or a content handler)
class NativeApplicationLoader : public mojo::NativeApplicationLoader {
 public:
  NativeApplicationLoader(
      Context* context,
      scoped_ptr<DynamicServiceRunnerFactory> runner_factory);
  ~NativeApplicationLoader() override;

  void RegisterContentHandler(const std::string& mime_type,
                              const GURL& content_handler_url);

  void Load(const GURL& url,
            InterfaceRequest<Application> application_request,
            LoadCallback callback) override;

 private:
  class Loader;
  class LocalLoader;
  class NetworkLoader;

  typedef std::map<std::string, GURL> MimeTypeToURLMap;
  typedef base::Callback<void(Loader*)> LoaderCompleteCallback;

  void LoaderComplete(Loader* loader);

  Context* const context_;
  scoped_ptr<DynamicServiceRunnerFactory> runner_factory_;
  NetworkServicePtr network_service_;
  MimeTypeToURLMap mime_type_to_url_;
  ScopedVector<Loader> loaders_;
  LoaderCompleteCallback loader_complete_callback_;

  DISALLOW_COPY_AND_ASSIGN(NativeApplicationLoader);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_NATIVE_APPLICATION_LOADER_H_
