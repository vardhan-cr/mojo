// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/reaper/reaper_binding.h"

#include "services/reaper/reaper_impl.h"

namespace reaper {

ReaperBinding::ReaperBinding(const GURL& caller_url,
                             ReaperImpl* impl,
                             mojo::InterfaceRequest<Reaper> request)
    : caller_url_(caller_url), impl_(impl), binding_(this, request.Pass()) {
}

void ReaperBinding::GetApplicationSecret(
    const mojo::Callback<void(uint64)>& callback) {
  impl_->GetApplicationSecret(caller_url_, callback);
}

void ReaperBinding::CreateReference(uint32 source_node, uint32 target_node) {
  impl_->CreateReference(caller_url_, source_node, target_node);
}

void ReaperBinding::DropNode(uint32 node) {
  impl_->DropNode(caller_url_, node);
}

}  // namespace reaper
