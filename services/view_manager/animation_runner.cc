// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/view_manager/animation_runner.h"

#include <set>

#include "services/view_manager/animation_runner_observer.h"
#include "services/view_manager/scheduled_animation_group.h"
#include "services/view_manager/server_view.h"

namespace view_manager {
namespace {

struct AnimationDoneState {
  ServerView* view;
  uint32_t animation_id;
};

}  // namespace

AnimationRunner::AnimationRunner(base::TimeTicks now)
    : next_id_(1), last_tick_time_(now) {
}

AnimationRunner::~AnimationRunner() {
}

void AnimationRunner::AddObserver(AnimationRunnerObserver* observer) {
  observers_.AddObserver(observer);
}

void AnimationRunner::RemoveObserver(AnimationRunnerObserver* observer) {
  observers_.RemoveObserver(observer);
}

uint32_t AnimationRunner::Schedule(
    ServerView* view,
    const mojo::AnimationGroup& transport_group) {
  scoped_ptr<ScheduledAnimationGroup> group(ScheduledAnimationGroup::Create(
      view, last_tick_time_, next_id_++, transport_group));
  if (!group.get())
    return 0;

  if (animation_map_.contains(view)) {
    animation_map_.get(view)->SetValuesToTargetValuesForPropertiesNotIn(*group);
    const uint32_t animation_id = animation_map_.get(view)->id();
    animation_map_.erase(view);
    FOR_EACH_OBSERVER(AnimationRunnerObserver, observers_,
                      OnAnimationInterrupted(view, animation_id));
  }

  group->ObtainStartValues();

  const uint32_t id = group->id();
  animation_map_.set(view, group.Pass());

  FOR_EACH_OBSERVER(AnimationRunnerObserver, observers_,
                    OnAnimationScheduled(view, id));
  return id;
}

ServerView* AnimationRunner::GetViewForAnimation(uint32_t id) {
  for (ViewAnimationMap::iterator i = animation_map_.begin();
       i != animation_map_.end(); ++i) {
    if (i->second->id() == id)
      return i->first;
  }
  return nullptr;
}

void AnimationRunner::CancelAnimationForView(ServerView* view) {
  if (!animation_map_.contains(view))
    return;

  const uint32_t id = animation_map_.get(view)->id();
  animation_map_.erase(view);
  FOR_EACH_OBSERVER(AnimationRunnerObserver, observers_,
                    OnAnimationCanceled(view, id));
}

void AnimationRunner::Tick(base::TimeTicks time) {
  DCHECK(time >= last_tick_time_);
  last_tick_time_ = time;
  if (animation_map_.empty())
    return;

  std::vector<AnimationDoneState> animations_done;
  for (ViewAnimationMap::iterator i = animation_map_.begin();
       i != animation_map_.end(); ) {
    // Any animations that complete are notified at the end of the loop. This
    // way if the obsevert attempts to schedule another animation or mutate us
    // in some other way we aren't in a bad state.
    if (i->second->Tick(time)) {
      AnimationDoneState done_state;
      done_state.view = i->first;
      done_state.animation_id = i->second->id();
      animations_done.push_back(done_state);
      animation_map_.erase(i++);
    } else {
      ++i;
    }
  }
  for (const AnimationDoneState& done : animations_done) {
    FOR_EACH_OBSERVER(AnimationRunnerObserver, observers_,
                      OnAnimationDone(done.view, done.animation_id));
  }
}

}  // namespace view_manager
