// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_TRACING_APP_H_
#define SERVICES_TRACING_TRACING_APP_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "mojo/common/interface_ptr_set.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/tracing/public/interfaces/tracing.mojom.h"
#include "services/tracing/trace_data_sink.h"
#include "services/tracing/trace_recorder_impl.h"

namespace tracing {

class TracingApp : public mojo::ApplicationDelegate,
                   public mojo::InterfaceFactory<TraceCollector>,
                   public TraceCollector {
 public:
  TracingApp();
  ~TracingApp() override;

 private:
  // mojo::ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // mojo::InterfaceFactory<TraceCollector> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<TraceCollector> request) override;

  // tracing::TraceCollector implementation.
  void Start(mojo::ScopedDataPipeProducerHandle stream,
             const mojo::String& categories) override;
  void StopAndFlush() override;

  void AllDataCollected();

  scoped_ptr<TraceDataSink> sink_;
  ScopedVector<TraceRecorderImpl> recorder_impls_;
  mojo::InterfacePtrSet<TraceProvider> provider_ptrs_;
  mojo::Binding<TraceCollector> collector_binding_;
  bool tracing_active_;
  mojo::String tracing_categories_;

  DISALLOW_COPY_AND_ASSIGN(TracingApp);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_TRACING_APP_H_
