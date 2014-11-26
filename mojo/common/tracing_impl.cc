// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/common/tracing_impl.h"

#include "base/callback.h"
#include "base/debug/trace_event.h"
#include "base/files/file_util.h"

namespace mojo {
namespace {

static bool g_tracing = false;
static int g_blocks = 0;
static FILE* g_trace_file = NULL;

void WriteTraceDataCollected(
    const scoped_refptr<base::RefCountedString>& events_str,
    bool has_more_events) {
  if (g_blocks) {
    fwrite(",", 1, 1, g_trace_file);
  }
  ++g_blocks;
  fwrite(events_str->data().c_str(), 1, events_str->data().length(),
         g_trace_file);
  if (!has_more_events) {
    fwrite("]}", 1, 2, g_trace_file);
    base::CloseFile(g_trace_file);
    g_trace_file = NULL;
    g_blocks = 0;
  }
}

void StopTracingAndFlushToDisk(base::FilePath::StringType base_name) {
  base::debug::TraceLog::GetInstance()->SetDisabled();

  base_name.append(FILE_PATH_LITERAL(".trace"));
  g_trace_file = base::OpenFile(base::FilePath(base_name), "w+");
  static const char start[] = "{\"traceEvents\":[";
  fwrite(start, 1, strlen(start), g_trace_file);
  base::debug::TraceLog::GetInstance()->Flush(
      base::Bind(&WriteTraceDataCollected));
}
}

TracingImpl::TracingImpl(InterfaceRequest<Tracing> request,
                         base::FilePath::StringType base_name)
    : base_name_(base_name), binding_(this, request.Pass()) {
}

TracingImpl::~TracingImpl() {
}

void TracingImpl::Start() {
  if (g_tracing)
    return;
  g_tracing = true;
  base::debug::TraceLog::GetInstance()->SetEnabled(
      base::debug::CategoryFilter("*"), base::debug::TraceLog::RECORDING_MODE,
      base::debug::TraceOptions(base::debug::RECORD_UNTIL_FULL));
}

void TracingImpl::Stop() {
  if (!g_tracing)
    return;
  g_tracing = false;
  StopTracingAndFlushToDisk(base_name_);
}

}  // namespace mojo
