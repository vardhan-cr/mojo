// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_COMMON_TRACING_IMPL_H_
#define MOJO_COMMON_TRACING_IMPL_H_

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "mojo/common/tracing.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace mojo {

class TracingImpl : public Tracing {
 public:
  TracingImpl(InterfaceRequest<Tracing> request,
              base::FilePath::StringType base_name);
  virtual ~TracingImpl();

 private:
  // Overridden from Tracing:
  void Start() override;
  void Stop() override;

  base::FilePath::StringType base_name_;
  StrongBinding<Tracing> binding_;

  DISALLOW_COPY_AND_ASSIGN(TracingImpl);
};

}  // namespace mojo

#endif  // MOJO_COMMON_TRACING_IMPL_H_
