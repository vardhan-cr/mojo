// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/measurements.h"

#include <algorithm>

#include "testing/gtest/include/gtest/gtest.h"

namespace benchmark {
namespace {

base::TimeTicks Ticks(int64 value) {
  return base::TimeTicks::FromInternalValue(value);
}

base::TimeDelta Delta(int64 value) {
  return base::TimeDelta::FromInternalValue(value);
}

class MeasurementsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    events_.resize(6);
    events_[0] = Event("a", "some", Ticks(10), Delta(2));
    events_[1] = Event("a", "some", Ticks(11), Delta(4));
    events_[2] = Event("a", "other", Ticks(12), Delta(8));
    events_[3] = Event("b", "some", Ticks(3), Delta(16));
    events_[4] = Event("b", "some", Ticks(13), Delta(32));
    // Event entries carrying metadata have timestamp of 0.
    events_[5] = Event("something", "__metadata", Ticks(0), Delta(0));

    reversed_ = events_;
    reverse(reversed_.begin(), reversed_.end());
  }

  std::vector<Event> events_;
  std::vector<Event> reversed_;
};

TEST_F(MeasurementsTest, MeasureTimeUntil) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  EXPECT_DOUBLE_EQ(0.008, regular.Measure(Measurement(
                              MeasurementType::TIME_UNTIL, "a", "some")));
  EXPECT_DOUBLE_EQ(0.008, reversed.Measure(Measurement(
                              MeasurementType::TIME_UNTIL, "a", "some")));

  EXPECT_DOUBLE_EQ(0.01, regular.Measure(Measurement(
                             MeasurementType::TIME_UNTIL, "a", "other")));
  EXPECT_DOUBLE_EQ(0.01, reversed.Measure(Measurement(
                             MeasurementType::TIME_UNTIL, "a", "other")));

  EXPECT_DOUBLE_EQ(0.001, regular.Measure(Measurement(
                              MeasurementType::TIME_UNTIL, "b", "some")));
  EXPECT_DOUBLE_EQ(0.001, reversed.Measure(Measurement(
                              MeasurementType::TIME_UNTIL, "b", "some")));
}

TEST_F(MeasurementsTest, MeasureAvgDuration) {
  // The results should be the same regardless of the order of events.
  Measurements regular(events_, base::TimeTicks::FromInternalValue(2));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(2));

  EXPECT_DOUBLE_EQ(0.003, regular.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "a", "some")));
  EXPECT_DOUBLE_EQ(0.003, reversed.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "a", "some")));

  EXPECT_DOUBLE_EQ(0.008, regular.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "a", "other")));
  EXPECT_DOUBLE_EQ(0.008, reversed.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "a", "other")));

  EXPECT_DOUBLE_EQ(0.024, regular.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "b", "some")));
  EXPECT_DOUBLE_EQ(0.024, reversed.Measure(Measurement(
                              MeasurementType::AVG_DURATION, "b", "some")));
}

TEST_F(MeasurementsTest, NoMatchingEvent) {
  // The results should be the same regardless of the order of events.
  Measurements empty(std::vector<Event>(),
                     base::TimeTicks::FromInternalValue(0));
  Measurements regular(events_, base::TimeTicks::FromInternalValue(0));
  Measurements reversed(reversed_, base::TimeTicks::FromInternalValue(0));

  EXPECT_DOUBLE_EQ(-1.0, empty.Measure(Measurement(
                             MeasurementType::AVG_DURATION, "miss", "cat")));
  EXPECT_DOUBLE_EQ(-1.0, regular.Measure(Measurement(
                             MeasurementType::AVG_DURATION, "miss", "cat")));
  EXPECT_DOUBLE_EQ(-1.0, reversed.Measure(Measurement(
                             MeasurementType::AVG_DURATION, "miss", "cat")));

  EXPECT_DOUBLE_EQ(-1.0, empty.Measure(Measurement(MeasurementType::TIME_UNTIL,
                                                   "miss", "cat")));
  EXPECT_DOUBLE_EQ(-1.0, regular.Measure(Measurement(
                             MeasurementType::TIME_UNTIL, "miss", "cat")));
  EXPECT_DOUBLE_EQ(-1.0, reversed.Measure(Measurement(
                             MeasurementType::TIME_UNTIL, "miss", "cat")));
}

}  // namespace

}  // namespace benchmark
