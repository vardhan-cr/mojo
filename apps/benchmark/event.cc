// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/event.h"

#include "base/json/json_reader.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"

namespace benchmark {

Event::Event() {}

Event::Event(std::string name,
             std::string category,
             base::TimeTicks timestamp,
             base::TimeDelta duration)
    : name(name),
      category(category),
      timestamp(timestamp),
      duration(duration) {}

Event::~Event() {}

bool GetEvents(const std::string& trace_json, std::vector<Event>* result) {
  result->clear();

  // Parse the JSON string describing the events.
  base::JSONReader reader;
  scoped_ptr<base::Value> trace_data = reader.ReadToValue(trace_json);
  if (!trace_data) {
    return false;
  }

  base::ListValue* event_list;
  if (!trace_data->GetAsList(&event_list))
    return false;

  for (base::Value* val : *event_list) {
    Event event;
    base::DictionaryValue* dict;
    if (!val->GetAsDictionary(&dict))
      return false;

    if (!dict->GetString("name", &event.name))
      return false;

    if (!dict->GetString("cat", &event.category))
      return false;

    double timestamp;
    if (!dict->GetDouble("ts", &timestamp))
      return false;
    event.timestamp = base::TimeTicks::FromInternalValue(timestamp);

    // It is valid for an event to not have duration.
    double duration;
    if (!dict->GetDouble("dur", &duration)) {
      event.duration = base::TimeDelta();
    } else {
      event.duration = base::TimeDelta::FromInternalValue(duration);
    }

    result->push_back(event);
  }
  return true;
}
}  // namespace benchmark
