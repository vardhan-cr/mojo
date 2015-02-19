// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/application_manager/application_loader.h"

#include "base/bind.h"
#include "base/logging.h"

namespace mojo {

namespace {

void NotReached(const GURL& url,
                InterfaceRequest<Application> application_request,
                URLResponsePtr response) {
  NOTREACHED();
}

}  // namespace

LoadCallback NativeApplicationLoader::SimpleLoadCallback() {
  return base::Bind(&NotReached);
}

}  // namespace mojo
