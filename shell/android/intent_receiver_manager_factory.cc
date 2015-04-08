// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/intent_receiver_manager_factory.h"

namespace mojo {
namespace shell {

void IntentReceiverManagerFactory::Create(
    ApplicationConnection* connection,
    InterfaceRequest<intent_receiver::IntentReceiverManager> request) {
  intent_manager_.Bind(request.Pass());
}

}  // namespace shell
}  // namespace mojo
