// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/benchmark/event.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"

namespace benchmark {

namespace {

TEST(GetEventsTest, Empty) {
  std::vector<Event> events;
  EXPECT_TRUE(GetEvents("[]", &events));
  EXPECT_EQ(0u, events.size());
}

TEST(GetEventsTest, Invalid) {
  std::vector<Event> events;
  EXPECT_FALSE(GetEvents("", &events));
  EXPECT_FALSE(GetEvents("[", &events));
  EXPECT_FALSE(GetEvents("]", &events));
  EXPECT_FALSE(GetEvents("[,]", &events));
}

TEST(GetEventsTest, Typical) {
  std::string event1 =
      "{\"pid\":7845,\"tid\":7845,\"ts\":1988539886444,"
      "\"ph\":\"X\",\"cat\":\"toplevel\",\"name\":\"MessageLoop::RunTask\","
      "\"args\":{\"src_file\":\"../../mojo/message_pump/handle_watcher.cc\","
      "\"src_func\":\"RemoveAndNotify\"},\"dur\":647,\"tdur\":633,"
      "\"tts\":25295}";
  std::string event2 =
      "{\"pid\":8238,\"tid\":8264,\"ts\":1988975370433,\"ph\":\"X\","
      "\"cat\":\"gpu\",\"name\":\"GLES2::WaitForCmd\",\"args\":{},\"dur\":7288,"
      "\"tdur\":212,\"tts\":6368}";

  std::string trace_json = "[" + event1 + "," + event2 + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(2u, events.size());

  EXPECT_EQ("MessageLoop::RunTask", events[0].name);
  EXPECT_EQ("toplevel", events[0].category);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1988539886444),
            events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(647), events[0].duration);

  EXPECT_EQ("GLES2::WaitForCmd", events[1].name);
  EXPECT_EQ("gpu", events[1].category);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1988975370433),
            events[1].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(7288), events[1].duration);
}

TEST(GetEventsTest, NoDuration) {
  std::string event =
      "{\"pid\":8238,\"tid\":8238,\"ts\":1988978463403,\"ph\":\"X\","
      "\"cat\":\"toplevel\",\"name\":\"MessageLoop::RunTask\",\"args\":"
      "{\"src_file\":\"../../mojo/message_pump/handle_watcher.cc\","
      "\"src_func\":\"RemoveAndNotify\"},\"tdur\":0,\"tts\":22812}";

  std::string trace_json = "[" + event + "]";
  std::vector<Event> events;
  ASSERT_TRUE(GetEvents(trace_json, &events));
  ASSERT_EQ(1u, events.size());

  EXPECT_EQ("MessageLoop::RunTask", events[0].name);
  EXPECT_EQ("toplevel", events[0].category);
  EXPECT_EQ(base::TimeTicks::FromInternalValue(1988978463403),
            events[0].timestamp);
  EXPECT_EQ(base::TimeDelta::FromInternalValue(0), events[0].duration);
}

}  // namespace

}  // namespace benchmark
