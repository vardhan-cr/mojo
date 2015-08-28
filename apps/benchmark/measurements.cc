// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/measurements.h"

namespace benchmark {
namespace {

bool Match(const Event& event, const EventSpec& spec) {
  return event.name == spec.name && event.category == spec.category;
}

}  // namespace

EventSpec::EventSpec() {}

EventSpec::EventSpec(std::string name, std::string category)
    : name(name), category(category) {}

EventSpec::~EventSpec() {}

Measurement::Measurement() {}

Measurement::Measurement(MeasurementType type,
                         std::string target_name,
                         std::string target_category)
    : type(type), target_event(target_name, target_category) {}

Measurement::~Measurement() {}

Measurements::Measurements(std::vector<Event> events,
                           base::TimeTicks time_origin)
    : events_(events), time_origin_(time_origin) {}

Measurements::~Measurements() {}

double Measurements::Measure(const Measurement& measurement) {
  switch (measurement.type) {
    case MeasurementType::TIME_UNTIL:
      return TimeUntil(measurement.target_event);
    case MeasurementType::AVG_DURATION:
      return AvgDuration(measurement.target_event);
    default:
      NOTREACHED();
      return double();
  }
}

double Measurements::TimeUntil(const EventSpec& event_spec) {
  base::TimeTicks earliest;
  bool found = false;
  for (const Event& event : events_) {
    if (event.category == "__metadata")
      continue;

    if (!Match(event, event_spec))
      continue;

    if (found) {
      earliest = std::min(earliest, event.timestamp);
    } else {
      earliest = event.timestamp;
      found = true;
    }
  }
  if (!found)
    return -1.0;
  return (earliest - time_origin_).InMillisecondsF();
}

double Measurements::AvgDuration(const EventSpec& event_spec) {
  double sum = 0.0;
  int count = 0;
  for (const Event& event : events_) {
    if (event.category == "__metadata")
      continue;

    if (!Match(event, event_spec))
      continue;

    sum += event.duration.InMillisecondsF();
    count += 1;
  }

  if (!count)
    return -1.0;
  return sum / count;
}

}  // namespace benchmark
