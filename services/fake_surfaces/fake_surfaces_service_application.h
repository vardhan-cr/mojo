// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FAKE_SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_
#define FAKE_SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_

#include "base/macros.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/surfaces/public/interfaces/display.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"

namespace mojo {
class ApplicationConnection;
}

namespace fake_surfaces {

class FakeSurfacesServiceApplication
    : public mojo::ApplicationDelegate,
      public mojo::InterfaceFactory<mojo::DisplayFactory>,
      public mojo::InterfaceFactory<mojo::Surface> {
 public:
  FakeSurfacesServiceApplication();
  ~FakeSurfacesServiceApplication() override;

  // ApplicationDelegate implementation.
  void Initialize(mojo::ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  // InterfaceFactory<mojo::DisplayFactory> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::DisplayFactory> request) override;

  // InterfaceFactory<mojo::Surface> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::Surface> request) override;

 private:
  uint32_t next_id_namespace_;
  mojo::TracingImpl tracing_;

  DISALLOW_COPY_AND_ASSIGN(FakeSurfacesServiceApplication);
};

}  // namespace fake_surfaces

#endif  // FAKE_SERVICES_SURFACES_SURFACES_SERVICE_APPLICATION_H_
