// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_REAPER_TRANSFER_BINDING_H_
#define SERVICES_REAPER_TRANSFER_BINDING_H_

#include "base/basictypes.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/reaper/reaper.mojom.h"
#include "services/reaper/transfer.mojom.h"

namespace reaper {

class ReaperImpl;

class TransferBinding : public Transfer {
 public:
  TransferBinding(uint32 node_id,
                  ReaperImpl* impl,
                  mojo::InterfaceRequest<Transfer> request);

 protected:
  ~TransferBinding() override = default;

 private:
  // Transfer
  void Complete(uint64 app_secret, uint32 node_id) override;
  void Ping(const mojo::Closure& callback) override;

  uint32 node_id_;
  ReaperImpl* impl_;
  mojo::StrongBinding<Transfer> binding_;
};

}  // namespace reaper

#endif  // SERVICES_REAPER_TRANSFER_BINDING_H_
