// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/ui_application_loader_android.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "shell/application_manager/application_manager.h"

namespace mojo {

UIApplicationLoader::UIApplicationLoader(
    scoped_ptr<ApplicationLoader> real_loader,
    base::MessageLoop* ui_message_loop)
    : loader_(real_loader.Pass()), ui_message_loop_(ui_message_loop) {
}

UIApplicationLoader::~UIApplicationLoader() {
  ui_message_loop_->PostTask(
      FROM_HERE, base::Bind(&UIApplicationLoader::ShutdownOnUIThread,
                            base::Unretained(this)));
}

void UIApplicationLoader::Load(
    ApplicationManager* manager,
    const GURL& url,
    InterfaceRequest<Application> application_request,
    LoadCallback callback) {
  DCHECK(application_request.is_pending());
  ui_message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&UIApplicationLoader::LoadOnUIThread, base::Unretained(this),
                 manager, url, base::Passed(&application_request)));
}

void UIApplicationLoader::OnApplicationError(ApplicationManager* manager,
                                             const GURL& url) {
  ui_message_loop_->PostTask(
      FROM_HERE, base::Bind(&UIApplicationLoader::OnApplicationErrorOnUIThread,
                            base::Unretained(this), manager, url));
}

void UIApplicationLoader::LoadOnUIThread(
    ApplicationManager* manager,
    const GURL& url,
    InterfaceRequest<Application> application_request) {
  loader_->Load(manager, url, application_request.Pass(), SimpleLoadCallback());
}

void UIApplicationLoader::OnApplicationErrorOnUIThread(
    ApplicationManager* manager,
    const GURL& url) {
  loader_->OnApplicationError(manager, url);
}

void UIApplicationLoader::ShutdownOnUIThread() {
  // Destroy |loader_| on the thread it's actually used on.
  loader_.reset();
}

}  // namespace mojo
