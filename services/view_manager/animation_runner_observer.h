// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_OBSERVER_H_
#define SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_OBSERVER_H_

namespace mojo {
namespace service {

class ServerView;

class AnimationRunnerObserver {
 public:
  virtual void OnAnimationScheduled(const ServerView* view, uint32_t id) = 0;
  virtual void OnAnimationDone(const ServerView* view, uint32_t id) = 0;
  virtual void OnAnimationInterrupted(const ServerView* view, uint32_t id) = 0;
  virtual void OnAnimationCanceled(const ServerView* view, uint32_t id) = 0;

 protected:
  virtual ~AnimationRunnerObserver() {}
};

}  // namespace service
}  // namespace mojo

#endif  // SERVICES_VIEW_MANAGER_ANIMATION_RUNNER_OBSERVER_H_
