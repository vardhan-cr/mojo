// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_INTENT_RECEIVER_MANAGER_IMPL_H_
#define SHELL_ANDROID_INTENT_RECEIVER_MANAGER_IMPL_H_

#include "base/android/jni_android.h"
#include "mojo/common/weak_binding_set.h"
#include "services/android/intent_receiver.mojom.h"

namespace shell {

class IntentReceiverManagerImpl
    : public intent_receiver::IntentReceiverManager {
 public:
  void Bind(
      mojo::InterfaceRequest<intent_receiver::IntentReceiverManager> request);

 private:
  // Implementation of intent_receiver::IntentReceiverManager
  void RegisterIntentReceiver(
      intent_receiver::IntentReceiverPtr receiver,
      const RegisterIntentReceiverCallback& callback) override;
  void RegisterActivityResultReceiver(
      intent_receiver::IntentReceiverPtr receiver,
      const RegisterActivityResultReceiverCallback& callback) override;

  mojo::WeakBindingSet<intent_receiver::IntentReceiverManager> bindings_;
};

bool RegisterIntentReceiverRegistry(JNIEnv* env);

}  // namespace shell

#endif  // SHELL_ANDROID_INTENT_RECEIVER_MANAGER_IMPL_H_
