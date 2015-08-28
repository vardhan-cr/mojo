// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_IPC_INIT_HELPER_MOJO_H_
#define UI_OZONE_PUBLIC_IPC_INIT_HELPER_MOJO_H_

#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "ui/ozone/public/ipc_init_helper_ozone.h"

namespace ui {

class IpcInitHelperMojo : public IpcInitHelperOzone {
 public:
  virtual void HostInitialize(mojo::ApplicationImpl* application) = 0;
  virtual bool HostConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) = 0;

  virtual void GpuInitialize(mojo::ApplicationImpl* application) = 0;
  virtual bool GpuConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) = 0;
};

}  // namespace ui

#endif
