// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_TRACE_DATA_SINK_H_
#define SERVICES_TRACING_TRACE_DATA_SINK_H_

#include <string>

#include "base/files/file_path.h"

namespace tracing {

class TraceDataSink {
 public:
  explicit TraceDataSink(base::FilePath base_name);
  ~TraceDataSink();

  void AddChunk(const std::string& json);
  void Flush();

 private:
  FILE* file_;
  bool empty_;

  DISALLOW_COPY_AND_ASSIGN(TraceDataSink);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_TRACE_DATA_SINK_H_
