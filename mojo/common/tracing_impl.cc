// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/tracing_impl.h"

#include "base/debug/trace_event.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"

namespace mojo {

// static
void TracingImpl::Create(ApplicationImpl* app) {
  new TracingImpl(app);
}

// static
void TracingImpl::Create(tracing::TraceDataCollectorPtr ptr) {
  new TracingImpl(ptr.Pass());
}

TracingImpl::TracingImpl(ApplicationImpl* app) : binding_(this) {
  ApplicationConnection* connection = app->ConnectToApplication("mojo:tracing");
  tracing::TraceDataCollectorPtr trace_data_collector_ptr;
  connection->ConnectToService(&trace_data_collector_ptr);
  binding_.Bind(trace_data_collector_ptr.PassMessagePipe());
}

TracingImpl::TracingImpl(tracing::TraceDataCollectorPtr ptr)
    : binding_(this, ptr.PassMessagePipe()) {
}

TracingImpl::~TracingImpl() {
}

void TracingImpl::StartTracing(const String& categories) {
  std::string categories_str = categories.To<std::string>();
  base::debug::TraceLog::GetInstance()->SetEnabled(
      base::debug::CategoryFilter(categories_str),
      base::debug::TraceLog::RECORDING_MODE,
      base::debug::TraceOptions(base::debug::RECORD_UNTIL_FULL));
}

void TracingImpl::StopTracing() {
  base::debug::TraceLog::GetInstance()->SetDisabled();

  base::debug::TraceLog::GetInstance()->Flush(
      base::Bind(&TracingImpl::SendChunk, base::Unretained(this)));
}

void TracingImpl::SendChunk(
    const scoped_refptr<base::RefCountedString>& events_str,
    bool has_more_events) {
  binding_.client()->DataCollected(mojo::String(events_str->data()));
  if (!has_more_events) {
    binding_.client()->EndTracing();
  }
}

}  // namespace mojo
