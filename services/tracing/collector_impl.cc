// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/collector_impl.h"

namespace tracing {

CollectorImpl::CollectorImpl(mojo::InterfaceRequest<TraceDataCollector> request,
                             TraceDataSink* sink)
    : sink_(sink), binding_(this, request.Pass()) {
}

CollectorImpl::~CollectorImpl() {
}

void CollectorImpl::TryRead() {
  binding_.WaitForIncomingMethodCall(MojoDeadline(0));
}

mojo::Handle CollectorImpl::TraceDataCollectorHandle() const {
  return binding_.handle();
}

void CollectorImpl::DataCollected(const mojo::String& json) {
  sink_->AddChunk(json.To<std::string>());
}

}  // namespace tracing
