// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/trace_data_sink.h"

#include "base/logging.h"

namespace tracing {
namespace {

const char kStart[] = "{\"traceEvents\":[";
const char kEnd[] = "]}";

void Write(const mojo::ScopedDataPipeProducerHandle& pipe,
           const char* string,
           uint32_t num_bytes) {
  CHECK_EQ(MOJO_RESULT_OK,
           mojo::WriteDataRaw(pipe.get(), string, &num_bytes,
                              MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));
}
}

TraceDataSink::TraceDataSink(mojo::ScopedDataPipeProducerHandle pipe)
    : pipe_(pipe.Pass()), empty_(true) {
  Write(pipe_, kStart, strlen(kStart));
}

TraceDataSink::~TraceDataSink() {
  if (pipe_.is_valid())
    Flush();
  DCHECK(!pipe_.is_valid());
}

void TraceDataSink::AddChunk(const std::string& json) {
  if (!empty_)
    Write(pipe_, ",", 1);
  empty_ = false;
  Write(pipe_, json.data(), json.length());
}

void TraceDataSink::Flush() {
  Write(pipe_, kEnd, strlen(kEnd));
  pipe_.reset();
}

}  // namespace tracing
