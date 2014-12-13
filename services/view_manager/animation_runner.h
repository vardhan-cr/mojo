// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_H_
#define SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/observer_list.h"
#include "base/time/time.h"

namespace mojo {
class AnimationGroup;
}

namespace view_manager {

class AnimationRunnerObserver;
class ScheduledAnimationGroup;
class ServerView;

// AnimationRunner is responsible for maintaing and running a set of animations.
// The animations are represented as a set of AnimationGroups. New animations
// are scheduled by way of Schedule(). A |view| may only have one animation
// running at a time. Schedule()ing a new animation implicitly cancels the
// outstanding animation. Animations progress by way of the Tick() function.
class AnimationRunner {
 public:
  explicit AnimationRunner(base::TimeTicks now);
  ~AnimationRunner();

  void AddObserver(AnimationRunnerObserver* observer);
  void RemoveObserver(AnimationRunnerObserver* observer);

  // Schedules an animation for |view|. If there is an existing animation in
  // progress for |view| it is canceled and any properties that were animating
  // but are no longer animating are set to their target value.
  // Returns 0 if |transport_group| is not valid.
  uint32_t Schedule(ServerView* view,
                    const mojo::AnimationGroup& transport_group);

  // Returns the view the animation identified by |id| was scheduled for.
  ServerView* GetViewForAnimation(uint32_t id);

  // Cancels the animation scheduled for |view|. Does nothing if there is no
  // animation scheduled for |view|. This does not change |view|. That is, any
  // in progress animations are stopped.
  void CancelAnimationForView(ServerView* view);

  // Advance the animations updating values appropriately.
  void Tick(base::TimeTicks time);

  // Returns true if there are animations currently scheduled.
  bool HasAnimations() const { return !animation_map_.empty(); }

 private:
  using ViewAnimationMap =
      base::ScopedPtrHashMap<ServerView*, ScheduledAnimationGroup>;

  uint32_t next_id_;

  base::TimeTicks last_tick_time_;

  ObserverList<AnimationRunnerObserver> observers_;

  ViewAnimationMap animation_map_;

  DISALLOW_COPY_AND_ASSIGN(AnimationRunner);
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_H_
