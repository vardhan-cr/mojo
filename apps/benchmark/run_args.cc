// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/run_args.h"

#include <algorithm>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"

namespace benchmark {
namespace {

bool GetMeasurement(const std::string& measurement_spec, Measurement* result) {
  // Measurements are described in the format:
  // <measurement type>/<event category>/<event name>.
  std::vector<std::string> parts;
  base::SplitString(measurement_spec, '/', &parts);
  if (parts.size() != 3) {
    LOG(ERROR) << "Could not parse the measurement description: "
               << measurement_spec;
    return false;
  }

  if (parts[0] == "time_until") {
    result->type = MeasurementType::TIME_UNTIL;
  } else if (parts[0] == "avg_duration") {
    result->type = MeasurementType::AVG_DURATION;
  } else {
    LOG(ERROR) << "Could not recognize the measurement type: " << parts[0];
    return false;
  }

  result->target_event.category = parts[1];
  result->target_event.name = parts[2];
  result->spec = measurement_spec;
  return true;
}

}  // namespace

RunArgs::RunArgs() {}

RunArgs::~RunArgs() {}

bool GetRunArgs(const std::vector<std::string>& input_args, RunArgs* result) {
  base::CommandLine command_line(input_args);

  result->app = command_line.GetSwitchValueASCII("app");
  if (result->app.empty()) {
    LOG(ERROR) << "The required --app argument is empty or missing.";
    return false;
  }

  std::string duration_str = command_line.GetSwitchValueASCII("duration");
  if (duration_str.empty()) {
    LOG(ERROR) << "The required --duration argument is empty or missing.";
    return false;
  }
  int duration_int;
  if (!base::StringToInt(duration_str, &duration_int) || duration_int <= 0) {
    LOG(ERROR) << "Could not parse the --duration value as a positive integer: "
               << duration_str;
    return false;
  }
  result->duration = base::TimeDelta::FromSeconds(duration_int);

  // All regular arguments (not switches, ie. not preceded by "--") describe
  // measurements.
  for (const std::string& measurement_spec : command_line.GetArgs()) {
    Measurement measurement;
    if (!GetMeasurement(measurement_spec, &measurement)) {
      return false;
    }
    result->measurements.push_back(measurement);
  }

  return true;
}

}  // namespace benchmark
