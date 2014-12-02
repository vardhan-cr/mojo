// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_COMMON_TRACING_IMPL_H_
#define MOJO_COMMON_TRACING_IMPL_H_

#include "base/basictypes.h"
#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/tracing/tracing.mojom.h"

namespace mojo {

class ApplicationImpl;

class TracingImpl : public tracing::TraceController {
 public:
  // This constructs a TracingImpl and connects to the tracing service. The
  // lifetime of the TracingImpl is bound to the connection to the tracing
  // service.
  static void Create(ApplicationImpl* app);

  // This constructs a TracingImpl around a tracing::TraceDataCollectorPtr
  // already bound to the service.
  static void Create(tracing::TraceDataCollectorPtr ptr);

  virtual ~TracingImpl();

 private:
  explicit TracingImpl(ApplicationImpl* app);
  explicit TracingImpl(tracing::TraceDataCollectorPtr ptr);

  // Overridden from tracing::TraceController:
  void StartTracing(const String& categories) override;
  void StopTracing() override;

  void SendChunk(const scoped_refptr<base::RefCountedString>& events_str,
                 bool has_more_events);

  StrongBinding<tracing::TraceController> binding_;

  DISALLOW_COPY_AND_ASSIGN(TracingImpl);
};

}  // namespace mojo

#endif  // MOJO_COMMON_TRACING_IMPL_H_
