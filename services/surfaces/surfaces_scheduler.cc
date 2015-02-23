// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/surfaces/surfaces_scheduler.h"

namespace surfaces {

SurfacesScheduler::Client::~Client() {
}

SurfacesScheduler::SurfacesScheduler(Client* client) : client_(client) {
  cc::SchedulerSettings settings;
  scheduler_ = cc::Scheduler::Create(
      this, settings, 0, base::MessageLoop::current()->task_runner(), nullptr,
      nullptr);
  scheduler_->SetCanStart();
  scheduler_->SetVisible(true);
  scheduler_->SetCanDraw(true);
  scheduler_->SetNeedsCommit();
}

SurfacesScheduler::~SurfacesScheduler() {
}

void SurfacesScheduler::SetNeedsDraw() {
  scheduler_->SetNeedsRedraw();
}

void SurfacesScheduler::OnVSyncParametersUpdated(base::TimeTicks timebase,
                                                 base::TimeDelta interval) {
  scheduler_->CommitVSyncParameters(timebase, interval);
}

void SurfacesScheduler::WillBeginImplFrame(const cc::BeginFrameArgs& args) {
}

void SurfacesScheduler::ScheduledActionSendBeginMainFrame() {
  scheduler_->NotifyBeginMainFrameStarted();
  scheduler_->NotifyReadyToCommit();
}

cc::DrawResult SurfacesScheduler::ScheduledActionDrawAndSwapIfPossible() {
  base::TimeTicks start = base::TimeTicks::Now();
  client_->Draw();
  base::TimeDelta duration = base::TimeTicks::Now() - start;

  draw_estimate_ = (duration + draw_estimate_) / 2;
  return cc::DRAW_SUCCESS;
}

cc::DrawResult SurfacesScheduler::ScheduledActionDrawAndSwapForced() {
  NOTREACHED() << "ScheduledActionDrawAndSwapIfPossible always succeeds.";
  return cc::DRAW_SUCCESS;
}

void SurfacesScheduler::ScheduledActionAnimate() {
}

void SurfacesScheduler::ScheduledActionCommit() {
}

void SurfacesScheduler::ScheduledActionActivateSyncTree() {
}

void SurfacesScheduler::ScheduledActionBeginOutputSurfaceCreation() {
  scheduler_->DidCreateAndInitializeOutputSurface();
}

void SurfacesScheduler::ScheduledActionPrepareTiles() {
}

void SurfacesScheduler::DidAnticipatedDrawTimeChange(base::TimeTicks time) {
}

base::TimeDelta SurfacesScheduler::DrawDurationEstimate() {
  return draw_estimate_;
}

base::TimeDelta SurfacesScheduler::BeginMainFrameToCommitDurationEstimate() {
  return base::TimeDelta();
}

base::TimeDelta SurfacesScheduler::CommitToActivateDurationEstimate() {
  return base::TimeDelta();
}

void SurfacesScheduler::DidBeginImplFrameDeadline() {
}

void SurfacesScheduler::SendBeginFramesToChildren(
    const cc::BeginFrameArgs& args) {
}

}  // namespace mojo
