// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/tracer.h"

#include <stdio.h>
#include <string.h>

#include "base/message_loop/message_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "base/trace_event/trace_event.h"

namespace shell {

Tracer::Tracer()
    : tracing_(false), first_chunk_written_(false), trace_file_(nullptr) {
}

Tracer::~Tracer() {
}

void Tracer::Start(const std::string& categories) {
  tracing_ = true;
  base::trace_event::CategoryFilter category_filter(categories);
  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      category_filter, base::trace_event::TraceLog::RECORDING_MODE,
      base::trace_event::TraceOptions(base::trace_event::RECORD_UNTIL_FULL));
}

void Tracer::StopAndFlushToFile(const std::string& filename) {
  if (tracing_)
    StopTracingAndFlushToDisk(filename);
}

void Tracer::EndTraceAndFlush(const std::string& filename,
                              const base::Closure& done_callback) {
  trace_file_ = fopen(filename.c_str(), "w+");
  PCHECK(trace_file_);
  static const char kStart[] = "{\"traceEvents\":[";
  fwrite(kStart, 1, strlen(kStart), trace_file_);
  base::trace_event::TraceLog::GetInstance()->SetDisabled();
  base::trace_event::TraceLog::GetInstance()->Flush(base::Bind(
      &Tracer::WriteTraceDataCollected, base::Unretained(this), done_callback));
}

void Tracer::WriteTraceDataCollected(
    const base::Closure& done_callback,
    const scoped_refptr<base::RefCountedString>& events_str,
    bool has_more_events) {
  if (events_str->size()) {
    if (first_chunk_written_)
      fwrite(",", 1, 1, trace_file_);

    first_chunk_written_ = true;
    fwrite(events_str->data().c_str(), 1, events_str->data().length(),
           trace_file_);
  }

  if (!has_more_events) {
    static const char kEnd[] = "]}";
    fwrite(kEnd, 1, strlen(kEnd), trace_file_);
    PCHECK(fclose(trace_file_) == 0);
    trace_file_ = nullptr;
    done_callback.Run();
  }
}

void Tracer::StopTracingAndFlushToDisk(const std::string& filename) {
  tracing_ = false;
  base::trace_event::TraceLog::GetInstance()->SetDisabled();
  base::WaitableEvent flush_complete_event(false, false);
  // TraceLog::Flush requires a message loop but we've already shut ours down.
  // Spin up a new thread to flush things out.
  base::Thread flush_thread("mojo_shell_trace_event_flush");
  flush_thread.Start();
  flush_thread.message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&Tracer::EndTraceAndFlush, base::Unretained(this), filename,
                 base::Bind(&base::WaitableEvent::Signal,
                            base::Unretained(&flush_complete_event))));
  flush_complete_event.Wait();
  LOG(INFO) << "Wrote trace data to " << filename;
}

}  // namespace shell
