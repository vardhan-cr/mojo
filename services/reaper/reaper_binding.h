// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_REAPER_REAPER_BINDING_H_
#define SERVICES_REAPER_REAPER_BINDING_H_

#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/reaper/reaper.mojom.h"
#include "url/gurl.h"

namespace reaper {

class ReaperImpl;

// ReaperBinding stores the authorative identity (as provided by the shell) of
// the connecting application, then forwards calls onto the real implementation.
class ReaperBinding : public Reaper {
 public:
  ReaperBinding(const GURL& caller_url,
                ReaperImpl* impl,
                mojo::InterfaceRequest<Reaper> request);

 protected:
  ~ReaperBinding() override = default;

 private:
  // Reaper
  void GetApplicationSecret(
      const mojo::Callback<void(uint64)>& callback) override;
  void CreateReference(uint32 source_node, uint32 target_node) override;
  void DropNode(uint32 node) override;

  GURL caller_url_;
  ReaperImpl* impl_;
  mojo::StrongBinding<Reaper> binding_;

  DISALLOW_COPY_AND_ASSIGN(ReaperBinding);
};

}  // namespace reaper

#endif  // SERVICES_REAPER_REAPER_BINDING_H_
