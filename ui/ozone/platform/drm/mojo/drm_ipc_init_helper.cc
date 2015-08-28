// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/services/ozone_drm_gpu/public/interfaces/ozone_drm_gpu.mojom.h"
#include "mojo/services/ozone_drm_host/public/interfaces/ozone_drm_host.mojom.h"
#include "ui/ozone/platform/drm/mojo/drm_gpu_delegate.h"
#include "ui/ozone/platform/drm/mojo/drm_gpu_impl.h"
#include "ui/ozone/platform/drm/mojo/drm_host_delegate.h"
#include "ui/ozone/platform/drm/mojo/drm_host_impl.h"
#include "ui/ozone/public/ipc_init_helper_mojo.h"

namespace ui {

class InterfaceFactoryDrmHost
    : public mojo::InterfaceFactory<mojo::OzoneDrmHost> {
  // mojo::InterfaceFactory implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::OzoneDrmHost> request) override;
};

class InterfaceFactoryDrmGpu
    : public mojo::InterfaceFactory<mojo::OzoneDrmGpu> {
  // mojo::InterfaceFactory<OzoneDrmGpu> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::OzoneDrmGpu> request) override;
};

void InterfaceFactoryDrmHost::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::OzoneDrmHost> request) {
  new MojoDrmHostImpl(request.Pass());
}

void InterfaceFactoryDrmGpu::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::OzoneDrmGpu> request) {
  new MojoDrmGpuImpl(request.Pass());
}

class DrmIpcInitHelperMojo : public IpcInitHelperMojo {
 public:
  DrmIpcInitHelperMojo();
  ~DrmIpcInitHelperMojo();

  void HostInitialize(mojo::ApplicationImpl* application) override;
  bool HostConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  void GpuInitialize(mojo::ApplicationImpl* application) override;
  bool GpuConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

 private:
  mojo::OzoneDrmHostPtr ozone_drm_host_;
  mojo::OzoneDrmGpuPtr ozone_drm_gpu_;
};

DrmIpcInitHelperMojo::DrmIpcInitHelperMojo() {}

DrmIpcInitHelperMojo::~DrmIpcInitHelperMojo() {}

void DrmIpcInitHelperMojo::HostInitialize(mojo::ApplicationImpl* application) {
  application->ConnectToService("mojo:native_viewport_service",
                                &ozone_drm_gpu_);
  new MojoDrmHostDelegate(ozone_drm_gpu_.get());
}

void DrmIpcInitHelperMojo::GpuInitialize(mojo::ApplicationImpl* application) {
  application->ConnectToService("mojo:native_viewport_service",
                                &ozone_drm_host_);
  new MojoDrmGpuDelegate(ozone_drm_host_.get());
}

bool DrmIpcInitHelperMojo::HostConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<mojo::OzoneDrmHost>(new InterfaceFactoryDrmHost());
  return true;
}

bool DrmIpcInitHelperMojo::GpuConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  connection->AddService<mojo::OzoneDrmGpu>(new InterfaceFactoryDrmGpu());
  return true;
}

// static
IpcInitHelperOzone* IpcInitHelperOzone::Create() {
  return new DrmIpcInitHelperMojo();
}

}  // namespace ui
