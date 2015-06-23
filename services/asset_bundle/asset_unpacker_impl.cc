// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/asset_bundle/asset_unpacker_impl.h"

#include "base/logging.h"
#include "services/asset_bundle/asset_unpacker_job.h"

namespace mojo {
namespace asset_bundle {

AssetUnpackerImpl::AssetUnpackerImpl(InterfaceRequest<AssetUnpacker> request)
    : binding_(this, request.Pass()) {
}

AssetUnpackerImpl::~AssetUnpackerImpl() {
}

void AssetUnpackerImpl::UnpackZipStream(ScopedDataPipeConsumerHandle zipped,
                                        InterfaceRequest<AssetBundle> request) {
  (new AssetUnpackerJob(request.Pass()))->Unpack(zipped.Pass());
}

}  // namespace asset_bundle
}  // namespace mojo
