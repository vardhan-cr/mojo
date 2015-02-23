// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_SERVICES_SURFACES_SURFACES_SCHEDULER_H_
#define MOJO_SERVICES_SURFACES_SURFACES_SCHEDULER_H_

#include "cc/scheduler/scheduler.h"

namespace surfaces {

class SurfacesScheduler : public cc::SchedulerClient {
 public:
  class Client {
   public:
    virtual void Draw() = 0;

   protected:
    virtual ~Client();
  };

  explicit SurfacesScheduler(Client* client);
  ~SurfacesScheduler();

  void SetNeedsDraw();

  void OnVSyncParametersUpdated(base::TimeTicks timebase,
                                base::TimeDelta interval);

 private:
  void WillBeginImplFrame(const cc::BeginFrameArgs& args) override;
  void ScheduledActionSendBeginMainFrame() override;
  cc::DrawResult ScheduledActionDrawAndSwapIfPossible() override;
  cc::DrawResult ScheduledActionDrawAndSwapForced() override;
  void ScheduledActionAnimate() override;
  void ScheduledActionCommit() override;
  void ScheduledActionActivateSyncTree() override;
  void ScheduledActionBeginOutputSurfaceCreation() override;
  void ScheduledActionPrepareTiles() override;
  void DidAnticipatedDrawTimeChange(base::TimeTicks time) override;
  base::TimeDelta DrawDurationEstimate() override;
  base::TimeDelta BeginMainFrameToCommitDurationEstimate() override;
  base::TimeDelta CommitToActivateDurationEstimate() override;
  void DidBeginImplFrameDeadline() override;
  void SendBeginFramesToChildren(const cc::BeginFrameArgs& args) override;

  Client* client_;
  scoped_ptr<cc::Scheduler> scheduler_;
  base::TimeDelta draw_estimate_;

  DISALLOW_COPY_AND_ASSIGN(SurfacesScheduler);
};

}  // namespace mojo

#endif  // MOJO_SERVICES_SURFACES_SURFACES_SCHEDULER_H_
