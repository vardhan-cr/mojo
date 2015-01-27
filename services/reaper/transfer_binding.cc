// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/reaper/transfer_binding.h"

#include "services/reaper/reaper_impl.h"

namespace reaper {

TransferBinding::TransferBinding(uint32 node_id,
                                 ReaperImpl* impl,
                                 mojo::InterfaceRequest<Transfer> request)
    : node_id_(node_id), impl_(impl), binding_(this, request.Pass()) {
}

void TransferBinding::Complete(uint64 dest_app_secret, uint32 dest_node_id) {
  impl_->CompleteTransfer(node_id_, dest_app_secret, dest_node_id);
}

void TransferBinding::Ping(const mojo::Closure& callback) {
  callback.Run();
}

}  // namespace reaper
