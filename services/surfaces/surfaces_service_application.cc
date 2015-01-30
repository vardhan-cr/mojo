// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/surfaces/surfaces_service_application.h"

#include "cc/surfaces/display.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "services/surfaces/surfaces_impl.h"

namespace surfaces {

SurfacesServiceApplication::SurfacesServiceApplication()
    : next_id_namespace_(1u), display_(nullptr) {
}

SurfacesServiceApplication::~SurfacesServiceApplication() {
}

void SurfacesServiceApplication::Initialize(mojo::ApplicationImpl* app) {
  mojo::TracingImpl::Create(app);
  scheduler_.reset(new SurfacesScheduler(this));
}

bool SurfacesServiceApplication::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService(this);
  return true;
}

void SurfacesServiceApplication::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::Surface> request) {
  new SurfacesImpl(&manager_, next_id_namespace_++, this, request.Pass());
}

void SurfacesServiceApplication::OnVSyncParametersUpdated(
    base::TimeTicks timebase,
    base::TimeDelta interval) {
  scheduler_->OnVSyncParametersUpdated(timebase, interval);
}

void SurfacesServiceApplication::FrameSubmitted() {
  scheduler_->SetNeedsDraw();
}

void SurfacesServiceApplication::SetDisplay(cc::Display* display) {
  display_ = display;
}

void SurfacesServiceApplication::OnDisplayBeingDestroyed(cc::Display* display) {
  if (display_ == display)
    SetDisplay(nullptr);
}

void SurfacesServiceApplication::Draw() {
  if (display_)
    display_->Draw();
}

}  // namespace surfaces

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new surfaces::SurfacesServiceApplication);
  return runner.Run(shell_handle);
}
