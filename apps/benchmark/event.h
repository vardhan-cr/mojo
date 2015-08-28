// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_EVENT_H_
#define APPS_BENCHMARK_EVENT_H_

#include <string>
#include <vector>

#include "base/time/time.h"
#include "base/values.h"

namespace benchmark {

// Represents a single trace event.
struct Event {
  std::string name;
  std::string category;
  base::TimeTicks timestamp;
  base::TimeDelta duration;

  Event();
  Event(std::string name,
        std::string category,
        base::TimeTicks timestamp,
        base::TimeDelta duration);
  ~Event();
};

// Parses a JSON string representing a list of trace events. Stores the outcome
// in |result| and returns true on success. Returns false on error.
bool GetEvents(const std::string& trace_json, std::vector<Event>* result);

}  // namespace benchmark
#endif  // APPS_BENCHMARK_EVENT_H_
