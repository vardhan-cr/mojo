// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_INTENT_RECEIVER_MANAGER_FACTORY_H_
#define SHELL_ANDROID_INTENT_RECEIVER_MANAGER_FACTORY_H_

#include "mojo/public/cpp/application/interface_factory.h"
#include "services/android/intent_receiver.mojom.h"
#include "shell/android/intent_receiver_manager_impl.h"

namespace mojo {
namespace shell {
class IntentReceiverManagerFactory
    : public InterfaceFactory<intent_receiver::IntentReceiverManager> {
 private:
  // From InterfaceFactory:
  void Create(ApplicationConnection* connection,
              InterfaceRequest<intent_receiver::IntentReceiverManager> request)
      override;

  IntentReceiverManagerImpl intent_manager_;
};
}  // namespace shell
}  // namespace mojo

#endif  // SHELL_ANDROID_INTENT_RECEIVER_MANAGER_FACTORY_H_
