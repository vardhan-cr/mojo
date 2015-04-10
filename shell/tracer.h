// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_TRACER_H_
#define SHELL_TRACER_H_

#include <stdio.h>

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted_memory.h"

namespace shell {

// Tracer collects tracing data from base/trace_event and from externally
// configured sources, aggregates it into a single stream, and writes it out to
// a file. It should be constructed very early in a process' lifetime before any
// initialization that may be interesting to trace has occured and be shut down
// as late as possible to capture as much initialization/shutdown code as
// possible.
class Tracer {
 public:
  Tracer();
  ~Tracer();

  // Starts tracing the current process with the given set of categories.
  void Start(const std::string& categories);

  // Stops tracing and flushes all collected trace data to the given filename.
  // Blocks until the file write is complete. May be called after the message
  // loop is shut down.
  void StopAndFlushToFile(const std::string& filename);

 private:
  void StopTracingAndFlushToDisk(const std::string& filename);

  // Called from the flush thread. When all data is collected this runs
  // |done_callback| on the flush thread.
  void EndTraceAndFlush(const std::string& filename,
                        const base::Closure& done_callback);

  // Called from the flush thread.
  void WriteTraceDataCollected(
      const base::Closure& done_callback,
      const scoped_refptr<base::RefCountedString>& events_str,
      bool has_more_events);

  // Whether we're currently tracing. Main thread only.
  bool tracing_;

  // Whether we've written the first chunk. Flush thread only.
  bool first_chunk_written_;

  // Trace file, if open. Flush thread only.
  FILE* trace_file_;

  DISALLOW_COPY_AND_ASSIGN(Tracer);
};

}  // namespace shell

#endif  // SHELL_TRACER_H_
