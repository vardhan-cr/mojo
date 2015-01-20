// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/animation_runner.h"

#include "base/strings/stringprintf.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/services/view_manager/public/interfaces/view_manager_constants.mojom.h"
#include "services/view_manager/animation_runner_observer.h"
#include "services/view_manager/scheduled_animation_group.h"
#include "services/view_manager/server_view.h"
#include "services/view_manager/test_server_view_delegate.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::TimeDelta;
using mojo::ANIMATION_PROPERTY_NONE;
using mojo::ANIMATION_PROPERTY_OPACITY;
using mojo::ANIMATION_PROPERTY_TRANSFORM;
using mojo::ANIMATION_TWEEN_TYPE_LINEAR;
using mojo::AnimationElement;
using mojo::AnimationGroup;
using mojo::AnimationProperty;
using mojo::AnimationSequence;
using mojo::AnimationTweenType;
using mojo::AnimationValue;
using mojo::AnimationValuePtr;
using mojo::Transform;

namespace view_manager {
namespace {

class TestAnimationRunnerObserver : public AnimationRunnerObserver {
 public:
  TestAnimationRunnerObserver() {}
  ~TestAnimationRunnerObserver() override {}

  std::vector<std::string>* changes() { return &changes_; }
  std::vector<uint32_t>* change_ids() { return &change_ids_; }

  void clear_changes() {
    changes_.clear();
    change_ids_.clear();
  }

  // AnimationRunnerDelgate:
  void OnAnimationScheduled(const ServerView* view, uint32_t id) override {
    change_ids_.push_back(id);
    changes_.push_back(base::StringPrintf(
        "scheduled view=%d,%d", view->id().connection_id, view->id().view_id));
  }
  void OnAnimationDone(const ServerView* view, uint32_t id) override {
    change_ids_.push_back(id);
    changes_.push_back(base::StringPrintf(
        "done view=%d,%d", view->id().connection_id, view->id().view_id));
  }
  void OnAnimationInterrupted(const ServerView* view, uint32_t id) override {
    change_ids_.push_back(id);
    changes_.push_back(base::StringPrintf(
        "interruped view=%d,%d", view->id().connection_id, view->id().view_id));
  }
  void OnAnimationCanceled(const ServerView* view, uint32_t id) override {
    change_ids_.push_back(id);
    changes_.push_back(base::StringPrintf(
        "canceled view=%d,%d", view->id().connection_id, view->id().view_id));
  }

 private:
  std::vector<uint32_t> change_ids_;
  std::vector<std::string> changes_;

  DISALLOW_COPY_AND_ASSIGN(TestAnimationRunnerObserver);
};

// Creates an AnimationValuePtr from the specified float value.
AnimationValuePtr FloatAnimationValue(float float_value) {
  AnimationValuePtr value(AnimationValue::New());
  value->float_value = float_value;
  return value.Pass();
}

// Creates an AnimationValuePtr from the specified transform.
AnimationValuePtr TransformAnimationValue(const gfx::Transform& transform) {
  AnimationValuePtr value(AnimationValue::New());
  value->transform = Transform::From(transform);
  return value.Pass();
}

// Adds an AnimationElement to |group|s last sequence with the specified value.
void AddElement(AnimationGroup* group,
                TimeDelta time,
                AnimationValuePtr start_value,
                AnimationValuePtr target_value,
                AnimationProperty property,
                AnimationTweenType tween_type) {
  AnimationSequence& sequence =
      *(group->sequences[group->sequences.size() - 1]);
  sequence.elements.push_back(AnimationElement::New());
  AnimationElement& element =
      *(sequence.elements[sequence.elements.size() - 1]);
  element.property = property;
  element.duration = time.InMicroseconds();
  element.tween_type = tween_type;
  element.start_value = start_value.Pass();
  element.target_value = target_value.Pass();
}

void AddOpacityElement(AnimationGroup* group,
                       TimeDelta time,
                       AnimationValuePtr start_value,
                       AnimationValuePtr target_value) {
  AddElement(group, time, start_value.Pass(), target_value.Pass(),
             ANIMATION_PROPERTY_OPACITY, ANIMATION_TWEEN_TYPE_LINEAR);
}

void AddTransformElement(AnimationGroup* group,
                         TimeDelta time,
                         AnimationValuePtr start_value,
                         AnimationValuePtr target_value) {
  AddElement(group, time, start_value.Pass(), target_value.Pass(),
             ANIMATION_PROPERTY_TRANSFORM, ANIMATION_TWEEN_TYPE_LINEAR);
}

void AddPauseElement(AnimationGroup* group, TimeDelta time) {
  AddElement(group, time, AnimationValuePtr(), AnimationValuePtr(),
             ANIMATION_PROPERTY_NONE, ANIMATION_TWEEN_TYPE_LINEAR);
}

void InitGroupForView(AnimationGroup* group,
                      const ViewId& id,
                      int cycle_count) {
  group->view_id = ViewIdToTransportId(id);
  group->sequences.push_back(AnimationSequence::New());
  group->sequences[group->sequences.size() - 1]->cycle_count = cycle_count;
}

}  // namespace

class AnimationRunnerTest : public testing::Test {
 public:
  AnimationRunnerTest()
      : initial_time_(base::TimeTicks::Now()), runner_(initial_time_) {
    runner_.AddObserver(&runner_observer_);
  }
  ~AnimationRunnerTest() override { runner_.RemoveObserver(&runner_observer_); }

 protected:
  const base::TimeTicks initial_time_;
  TestAnimationRunnerObserver runner_observer_;
  AnimationRunner runner_;

 private:
  DISALLOW_COPY_AND_ASSIGN(AnimationRunnerTest);
};

// Opacity from 1 to .5 over 1000.
TEST_F(AnimationRunnerTest, SingleProperty) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId());

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  const uint32_t animation_id = runner_.Schedule(&view, group, initial_time_);

  ASSERT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("scheduled view=0,0", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));
  runner_observer_.clear_changes();

  EXPECT_TRUE(runner_.HasAnimations());

  // Opacity should still be 1 (the initial value).
  EXPECT_EQ(1.f, view.opacity());

  // Animate half way.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));

  EXPECT_EQ(.75f, view.opacity());
  EXPECT_TRUE(runner_observer_.changes()->empty());

  // Run well past the end. Value should progress to end and delegate should
  // be notified.
  runner_.Tick(initial_time_ + TimeDelta::FromSeconds(10));
  EXPECT_EQ(.5f, view.opacity());

  ASSERT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("done view=0,0", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));

  EXPECT_FALSE(runner_.HasAnimations());
}

// Opacity from 1 to .5, followed by transform from identity to 2x,3x.
TEST_F(AnimationRunnerTest, TwoPropertiesInSequence) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId());

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(.5f));

  gfx::Transform done_transform;
  done_transform.Scale(2, 4);
  AddTransformElement(&group, TimeDelta::FromMicroseconds(2000),
                      AnimationValuePtr(),
                      TransformAnimationValue(done_transform));

  const uint32_t animation_id = runner_.Schedule(&view, group, initial_time_);
  runner_observer_.clear_changes();

  // Nothing in the view should have changed yet.
  EXPECT_EQ(1.f, view.opacity());
  EXPECT_TRUE(view.transform().IsIdentity());

  // Animate half way from through opacity animation.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));

  EXPECT_EQ(.75f, view.opacity());
  EXPECT_TRUE(view.transform().IsIdentity());

  // Finish first element (opacity).
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(1000));
  EXPECT_EQ(.5f, view.opacity());
  EXPECT_TRUE(view.transform().IsIdentity());

  // Half way through second (transform).
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(2000));
  EXPECT_EQ(.5f, view.opacity());
  gfx::Transform half_way_transform;
  half_way_transform.Scale(1.5, 2.5);
  EXPECT_EQ(half_way_transform, view.transform());

  EXPECT_TRUE(runner_observer_.changes()->empty());

  // To end.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(3500));
  EXPECT_EQ(.5f, view.opacity());
  EXPECT_EQ(done_transform, view.transform());

  ASSERT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("done view=0,0", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));
}

// Opacity from .5 to 1 over 1000, transform to 2x,4x over 500.
TEST_F(AnimationRunnerTest, TwoPropertiesInParallel) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId(1, 1));

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    FloatAnimationValue(.5f), FloatAnimationValue(1));

  group.sequences.push_back(AnimationSequence::New());
  group.sequences[1]->cycle_count = 1;
  gfx::Transform done_transform;
  done_transform.Scale(2, 4);
  AddTransformElement(&group, TimeDelta::FromMicroseconds(500),
                      AnimationValuePtr(),
                      TransformAnimationValue(done_transform));

  const uint32_t animation_id = runner_.Schedule(&view, group, initial_time_);

  runner_observer_.clear_changes();

  // Nothing in the view should have changed yet.
  EXPECT_EQ(1.f, view.opacity());
  EXPECT_TRUE(view.transform().IsIdentity());

  // Animate to 250, which is 1/4 way through opacity and half way through
  // transform.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(250));

  EXPECT_EQ(.625f, view.opacity());
  gfx::Transform half_way_transform;
  half_way_transform.Scale(1.5, 2.5);
  EXPECT_EQ(half_way_transform, view.transform());

  // Animate to 500, which is 1/2 way through opacity and transform done.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));
  EXPECT_EQ(.75f, view.opacity());
  EXPECT_EQ(done_transform, view.transform());

  // Animate to 750, which is 3/4 way through opacity and transform done.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(750));
  EXPECT_EQ(.875f, view.opacity());
  EXPECT_EQ(done_transform, view.transform());

  EXPECT_TRUE(runner_observer_.changes()->empty());

  // To end.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(3500));
  EXPECT_EQ(1.f, view.opacity());
  EXPECT_EQ(done_transform, view.transform());

  ASSERT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("done view=1,1", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));
}

// Opacity from .5 to 1 over 1000, pause for 500, 1 to .5 over 500, with a cycle
// count of 3.
TEST_F(AnimationRunnerTest, Cycles) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId(1, 2));

  view.SetOpacity(.5f);

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 3);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(1));
  AddPauseElement(&group, TimeDelta::FromMicroseconds(500));
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(500),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  runner_.Schedule(&view, group, initial_time_);
  runner_observer_.clear_changes();

  // Nothing in the view should have changed yet.
  EXPECT_EQ(.5f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));
  EXPECT_EQ(.75f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(1250));
  EXPECT_EQ(1.f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(1750));
  EXPECT_EQ(.75f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(2500));
  EXPECT_EQ(.75f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(3250));
  EXPECT_EQ(1.f, view.opacity());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(3750));
  EXPECT_EQ(.75f, view.opacity());

  // Animate to the end.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(6500));
  EXPECT_EQ(.5f, view.opacity());

  ASSERT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("done view=1,2", runner_observer_.changes()->at(0));
}

// Verifies scheduling the same view twice sends an interrupt.
TEST_F(AnimationRunnerTest, ScheduleTwice) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId(1, 2));

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  const uint32_t animation_id = runner_.Schedule(&view, group, initial_time_);
  runner_observer_.clear_changes();

  // Animate half way.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));

  EXPECT_EQ(.75f, view.opacity());
  EXPECT_TRUE(runner_observer_.changes()->empty());

  // Schedule again. We should get an interrupt, but opacity shouldn't change.
  const uint32_t animation2_id = runner_.Schedule(
      &view, group, initial_time_ + base::TimeDelta::FromMicroseconds(500));

  // Id should have changed.
  EXPECT_NE(animation_id, animation2_id);

  EXPECT_EQ(nullptr, runner_.GetViewForAnimation(animation_id));
  EXPECT_EQ(&view, runner_.GetViewForAnimation(animation2_id));

  EXPECT_EQ(.75f, view.opacity());
  EXPECT_EQ(2u, runner_observer_.changes()->size());
  EXPECT_EQ("interruped view=1,2", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));
  EXPECT_EQ("scheduled view=1,2", runner_observer_.changes()->at(1));
  EXPECT_EQ(animation2_id, runner_observer_.change_ids()->at(1));
  runner_observer_.clear_changes();

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(1000));
  EXPECT_EQ(.625f, view.opacity());
  EXPECT_TRUE(runner_observer_.changes()->empty());

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(2000));
  EXPECT_EQ(.5f, view.opacity());
  EXPECT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("done view=1,2", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation2_id, runner_observer_.change_ids()->at(0));
}

// Verifies Remove() works.
TEST_F(AnimationRunnerTest, CancelAnimationForView) {
  // Create an animation and advance it part way.
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId());
  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  const uint32_t animation_id = runner_.Schedule(&view, group, initial_time_);
  runner_observer_.clear_changes();
  EXPECT_EQ(&view, runner_.GetViewForAnimation(animation_id));

  EXPECT_TRUE(runner_.HasAnimations());

  // Animate half way.
  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(500));
  EXPECT_EQ(.75f, view.opacity());
  EXPECT_TRUE(runner_observer_.changes()->empty());

  // Cancel the animation.
  runner_.CancelAnimationForView(&view);

  EXPECT_FALSE(runner_.HasAnimations());
  EXPECT_EQ(nullptr, runner_.GetViewForAnimation(animation_id));

  EXPECT_EQ(.75f, view.opacity());

  EXPECT_EQ(1u, runner_observer_.changes()->size());
  EXPECT_EQ("canceled view=0,0", runner_observer_.changes()->at(0));
  EXPECT_EQ(animation_id, runner_observer_.change_ids()->at(0));
}

// Verifies a tick with a very large delta and a sequence that repeats forever
// doesn't take a long time.
TEST_F(AnimationRunnerTest, InfiniteRepeatWithHugeGap) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId(1, 2));

  view.SetOpacity(.5f);

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 0);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(500),
                    AnimationValuePtr(), FloatAnimationValue(1));
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(500),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  runner_.Schedule(&view, group, initial_time_);
  runner_observer_.clear_changes();

  runner_.Tick(initial_time_ + TimeDelta::FromMicroseconds(1000000000750));

  EXPECT_EQ(.75f, view.opacity());

  ASSERT_EQ(0u, runner_observer_.changes()->size());
}

// Verifies a second schedule sets any properties that are no longer animating
// to their final value.
TEST_F(AnimationRunnerTest, RescheduleSetsPropertiesToFinalValue) {
  TestServerViewDelegate view_delegate;
  ServerView view(&view_delegate, ViewId());

  AnimationGroup group;
  InitGroupForView(&group, view.id(), 1);
  AddOpacityElement(&group, TimeDelta::FromMicroseconds(1000),
                    AnimationValuePtr(), FloatAnimationValue(.5));

  gfx::Transform done_transform;
  done_transform.Scale(2, 4);
  AddTransformElement(&group, TimeDelta::FromMicroseconds(500),
                      AnimationValuePtr(),
                      TransformAnimationValue(done_transform));

  runner_.Schedule(&view, group, initial_time_);

  // Schedule() again, this time without animating opacity.
  group.sequences[0]->elements[0]->property = ANIMATION_PROPERTY_NONE;
  runner_.Schedule(&view, group, initial_time_);

  // Opacity should go to final value.
  EXPECT_EQ(.5f, view.opacity());
  // Transform shouldn't have changed since newly scheduled animation also has
  // transform in it.
  EXPECT_TRUE(view.transform().IsIdentity());
}

}  // namespace view_manager
