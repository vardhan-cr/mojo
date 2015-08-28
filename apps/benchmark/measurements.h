// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_BENCHMARK_MEASUREMENTS_HH_
#define APPS_BENCHMARK_MEASUREMENTS_HH_

#include <vector>

#include "apps/benchmark/event.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/time/time.h"
#include "base/values.h"

namespace benchmark {

// Describes a trace event to match.
struct EventSpec {
  std::string name;
  std::string category;

  EventSpec();
  EventSpec(std::string name, std::string category);
  ~EventSpec();
};

enum class MeasurementType { TIME_UNTIL, AVG_DURATION };

// Represents a single measurement to be performed on the collected trace.
struct Measurement {
  MeasurementType type;
  EventSpec target_event;
  // Optional string from which this measurement was parsed. Can be used for
  // presentation purposes.
  std::string spec;

  Measurement();
  Measurement(MeasurementType type,
              std::string target_name,
              std::string target_category);
  ~Measurement();
};

class Measurements {
 public:
  Measurements(std::vector<Event> events, base::TimeTicks time_origin);
  ~Measurements();

  // Performs the given measurement. Returns the result in milliseconds or -1.0
  // if the measurement failed, e.g. because no events were matched.
  double Measure(const Measurement& measurement);

 private:
  double TimeUntil(const EventSpec& event_spec);
  double AvgDuration(const EventSpec& event_spec);

  std::vector<Event> events_;
  base::TimeTicks time_origin_;
  DISALLOW_COPY_AND_ASSIGN(Measurements);
};

}  // namespace benchmark

#endif  // APPS_BENCHMARK_MEASUREMENTS_HH_
