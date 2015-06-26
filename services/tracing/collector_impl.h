// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_COLLECTOR_IMPL_H_
#define SERVICES_TRACING_COLLECTOR_IMPL_H_

#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/string.h"
#include "mojo/services/tracing/public/interfaces/tracing.mojom.h"
#include "services/tracing/trace_data_sink.h"

namespace tracing {

class CollectorImpl : public TraceDataCollector {
 public:
  CollectorImpl(mojo::InterfaceRequest<TraceDataCollector> request,
                TraceDataSink* sink);
  ~CollectorImpl() override;

  // TryRead attempts to read a single chunk from the TraceDataCollector pipe if
  // one is available and passes it to the TraceDataSink. Returns immediately if
  // no chunk is available.
  void TryRead();

  // TraceDataCollectorHandle returns the handle value of the TraceDataCollector
  // binding which can be used to wait until chunks are available.
  mojo::Handle TraceDataCollectorHandle() const;

 private:
  // tracing::TraceDataCollector implementation.
  void DataCollected(const mojo::String& json) override;

  TraceDataSink* sink_;
  mojo::Binding<TraceDataCollector> binding_;

  DISALLOW_COPY_AND_ASSIGN(CollectorImpl);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_COLLECTOR_IMPL_H_
