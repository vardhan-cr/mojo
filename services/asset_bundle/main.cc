// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "services/asset_bundle/asset_unpacker_impl.h"

namespace mojo {
namespace asset_bundle {

class AssetBundleApp : public ApplicationDelegate,
                       public InterfaceFactory<AssetUnpacker> {
 public:
  AssetBundleApp() {}
  ~AssetBundleApp() override {}

 private:
  // |ApplicationDelegate| override:
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService<AssetUnpacker>(this);
    return true;
  }

  // |InterfaceFactory<AssetUnpacker>| implementation:
  void Create(ApplicationConnection* connection,
              InterfaceRequest<AssetUnpacker> request) override {
    new AssetUnpackerImpl(request.Pass());
  }

  DISALLOW_COPY_AND_ASSIGN(AssetBundleApp);
};

}  // namespace asset_bundle
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(
      new mojo::asset_bundle::AssetBundleApp());
  return runner.Run(application_request);
}
