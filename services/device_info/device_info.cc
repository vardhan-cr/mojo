// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdlib>

#include "mojo/common/binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/services/device_info/public/interfaces/device_info.mojom.h"

namespace mojo {
namespace services {
namespace device_info {

// This is a native Mojo application which implements |DeviceInfo| interface for
// Linux.
class DeviceInfo : public mojo::ApplicationDelegate,
                   public mojo::DeviceInfo,
                   public mojo::InterfaceFactory<mojo::DeviceInfo> {
 public:
  // We look for the 'DISPLAY' environment variable. If present, then we assume
  // it to be a desktop, else we assume it to be a commandline
  void GetDeviceType(const GetDeviceTypeCallback& callback) override {
    callback.Run(getenv("DISPLAY") ? DeviceInfo::DeviceType::DESKTOP
                                   : DeviceInfo::DeviceType::HEADLESS);
  }

  // |ApplicationDelegate| override.
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<mojo::DeviceInfo>(this);
    return true;
  }

  // |InterfaceFactory<DeviceInfo>| implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<mojo::DeviceInfo> request) override {
    binding_.AddBinding(this, request.Pass());
  }

 private:
  mojo::BindingSet<mojo::DeviceInfo> binding_;
};

}  // namespace device_info
}  // namespace services
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new mojo::services::device_info::DeviceInfo());
  return runner.Run(application_request);
}
