// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/trace_data_sink.h"

#include "base/files/file_util.h"

namespace tracing {

TraceDataSink::TraceDataSink(base::FilePath base_name) : empty_(true) {
  file_ =
      base::OpenFile(base_name.AddExtension(FILE_PATH_LITERAL(".trace")), "w+");
  static const char start[] = "{\"traceEvents\":[";
  fwrite(start, 1, strlen(start), file_);
}

TraceDataSink::~TraceDataSink() {
  if (file_)
    Flush();
  DCHECK(!file_);
}

void TraceDataSink::AddChunk(const std::string& json) {
  if (!empty_) {
    fwrite(",", 1, 1, file_);
  }

  empty_ = false;

  fwrite(json.data(), 1, json.length(), file_);
}

void TraceDataSink::Flush() {
  static const char end[] = "]}";
  fwrite(end, 1, strlen(end), file_);
  base::CloseFile(file_);
  file_ = nullptr;
}

}  // namespace tracing
