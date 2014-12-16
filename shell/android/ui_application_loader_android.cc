// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/android/ui_application_loader_android.h"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "mojo/application_manager/application_manager.h"

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

void UIApplicationLoader::Load(ApplicationManager* manager,
                               const GURL& url,
                               ScopedMessagePipeHandle shell_handle,
                               LoadCallback callback) {
  DCHECK(shell_handle.is_valid());
  ui_message_loop_->PostTask(
      FROM_HERE,
      base::Bind(&UIApplicationLoader::LoadOnUIThread, base::Unretained(this),
                 manager, url, base::Passed(&shell_handle)));
}

void UIApplicationLoader::OnApplicationError(ApplicationManager* manager,
                                             const GURL& url) {
  ui_message_loop_->PostTask(
      FROM_HERE, base::Bind(&UIApplicationLoader::OnApplicationErrorOnUIThread,
                            base::Unretained(this), manager, url));
}

void UIApplicationLoader::LoadOnUIThread(ApplicationManager* manager,
                                         const GURL& url,
                                         ScopedMessagePipeHandle shell_handle) {
  loader_->Load(manager, url, shell_handle.Pass(), SimpleLoadCallback());
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
