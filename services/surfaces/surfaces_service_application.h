// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_
#define SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "cc/surfaces/surface_manager.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "services/surfaces/surfaces_impl.h"
#include "services/surfaces/surfaces_scheduler.h"

namespace mojo {
class ApplicationConnection;
}

namespace surfaces {

class SurfacesServiceApplication
    : public mojo::ApplicationDelegate,
      public mojo::InterfaceFactory<mojo::Surface>,
      public SurfacesImpl::Client,
      public SurfacesScheduler::Client {
 public:
  SurfacesServiceApplication();
  ~SurfacesServiceApplication() override;

  // ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // InterfaceFactory<Surface> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::Surface> request) override;

  // SurfacesImpl::Client implementation.
  void OnVSyncParametersUpdated(base::TimeTicks timebase,
                                base::TimeDelta interval) override;
  void FrameSubmitted() override;
  void SetDisplay(cc::Display*) override;
  void OnDisplayBeingDestroyed(cc::Display* display) override;

  // SurfacesScheduler::Client
  void Draw() override;

 private:
  cc::SurfaceManager manager_;
  uint32_t next_id_namespace_;
  cc::Display* display_;
  scoped_ptr<SurfacesScheduler> scheduler_;
  mojo::TracingImpl tracing_;

  DISALLOW_COPY_AND_ASSIGN(SurfacesServiceApplication);
};

}  // namespace surfaces

#endif  //  SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_
