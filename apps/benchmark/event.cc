// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/event.h"

#include <map>
#include <stack>

#include "base/json/json_reader.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"

namespace benchmark {

Event::Event() {}

Event::Event(EventType type,
             std::string name,
             std::string categories,
             base::TimeTicks timestamp,
             base::TimeDelta duration)
    : type(type),
      name(name),
      categories(categories),
      timestamp(timestamp),
      duration(duration) {}

Event::~Event() {}

namespace {

// Finds the matching "duration end" event for each "duration begin" event and
// rewrites the "begin event" as a "complete" event with a duration. Leaves all
// events that are not "duration begin" unchanged.
bool JoinDurationEvents(base::ListValue* event_list) {
  // Maps thread ids to stacks of unmatched duration begin events on the given
  // thread.
  std::map<int, std::stack<base::DictionaryValue*>> open_begin_events;

  for (base::Value* val : *event_list) {
    base::DictionaryValue* event_dict;
    if (!val->GetAsDictionary(&event_dict))
      return false;

    std::string phase;
    if (!event_dict->GetString("ph", &phase)) {
      LOG(ERROR) << "Incorrect trace event (missing phase)";
      return false;
    }

    if (phase != "B" && phase != "E")
      continue;

    int tid;
    if (!event_dict->GetInteger("tid", &tid)) {
      LOG(ERROR) << "Incorrect trace event (missing tid).";
      return false;
    }

    if (phase == "B") {
      open_begin_events[tid].push(event_dict);
    } else if (phase == "E") {
      if (!open_begin_events.count(tid) || open_begin_events[tid].empty()) {
        LOG(ERROR) << "Incorrect trace event (duration end without begin).";
        return false;
      }

      base::DictionaryValue* begin_event_dict = open_begin_events[tid].top();
      open_begin_events[tid].pop();
      double begin_ts;
      if (!begin_event_dict->GetDouble("ts", &begin_ts)) {
        LOG(ERROR) << "Incorrect trace event (no timestamp)";
        return false;
      }

      double end_ts;
      if (!event_dict->GetDouble("ts", &end_ts)) {
        LOG(ERROR) << "Incorrect trace event (no timestamp)";
        return false;
      }

      if (end_ts < begin_ts) {
        LOG(ERROR) << "Incorrect trace event (duration timestamps decreasing)";
        return false;
      }

      begin_event_dict->SetDouble("dur", end_ts - begin_ts);
      begin_event_dict->SetString("ph", "X");
    } else {
      NOTREACHED();
    }
  }
  return true;
}

bool ParseEvents(base::ListValue* event_list, std::vector<Event>* result) {
  result->clear();

  for (base::Value* val : *event_list) {
    base::DictionaryValue* event_dict;
    if (!val->GetAsDictionary(&event_dict))
      return false;

    Event event;

    std::string phase;
    if (!event_dict->GetString("ph", &phase)) {
      LOG(ERROR) << "Incorrect trace event (missing phase)";
      return false;
    }
    if (phase == "X") {
      event.type = EventType::COMPLETE;
    } else if (phase == "I") {
      event.type = EventType::INSTANT;
    } else {
      // Skip all event types we do not handle.
      continue;
    }

    if (!event_dict->GetString("name", &event.name)) {
      LOG(ERROR) << "Incorrect trace event (no name)";
      return false;
    }

    if (!event_dict->GetString("cat", &event.categories)) {
      LOG(ERROR) << "Incorrect trace event (no categories)";
      return false;
    }

    double timestamp;
    if (!event_dict->GetDouble("ts", &timestamp)) {
      LOG(ERROR) << "Incorrect trace event (no timestamp)";
      return false;
    }
    event.timestamp = base::TimeTicks::FromInternalValue(timestamp);

    if (event.type == EventType::COMPLETE) {
      double duration;
      if (!event_dict->GetDouble("dur", &duration)) {
        LOG(ERROR) << "Incorrect complete or duration event (no duration)";
        return false;
      }

      event.duration = base::TimeDelta::FromInternalValue(duration);
    } else {
      event.duration = base::TimeDelta();
    }

    result->push_back(event);
  }
  return true;
}

}  // namespace

bool GetEvents(const std::string& trace_json, std::vector<Event>* result) {
  // Parse the JSON string describing the events.
  base::JSONReader reader;
  scoped_ptr<base::Value> trace_data = reader.ReadToValue(trace_json);
  if (!trace_data) {
    LOG(ERROR) << "Failed to parse the JSON string: "
               << reader.GetErrorMessage();
    return false;
  }

  base::ListValue* event_list;
  if (!trace_data->GetAsList(&event_list)) {
    LOG(ERROR) << "Incorrect format of the trace data.";
    return false;
  }

  if (!JoinDurationEvents(event_list))
    return false;

  if (!ParseEvents(event_list, result))
    return false;
  return true;
}
}  // namespace benchmark
