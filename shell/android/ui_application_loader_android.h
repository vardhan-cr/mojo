// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_
#define SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_

#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "shell/application_manager/application_loader.h"

namespace base {
class MessageLoop;
}

namespace mojo {

class ApplicationManager;

// ApplicationLoader implementation that creates a background thread and issues
// load
// requests there.
class UIApplicationLoader : public ApplicationLoader {
 public:
  UIApplicationLoader(scoped_ptr<ApplicationLoader> real_loader,
                      base::MessageLoop* ui_message_loop);
  ~UIApplicationLoader() override;

  // ApplicationLoader overrides:
  void Load(ApplicationManager* manager,
            const GURL& url,
            ShellPtr shell,
            LoadCallback callback) override;
  void OnApplicationError(ApplicationManager* manager,
                          const GURL& url) override;

 private:
  class UILoader;

  // These functions are exected on the background thread. They call through
  // to |background_loader_| to do the actual loading.
  // TODO: having this code take a |manager| is fragile (as ApplicationManager
  // isn't thread safe).
  void LoadOnUIThread(ApplicationManager* manager,
                      const GURL& url,
                      ShellPtr shell);
  void OnApplicationErrorOnUIThread(ApplicationManager* manager,
                                    const GURL& url);
  void ShutdownOnUIThread();

  scoped_ptr<ApplicationLoader> loader_;
  base::MessageLoop* ui_message_loop_;

  DISALLOW_COPY_AND_ASSIGN(UIApplicationLoader);
};

}  // namespace mojo

#endif  // SHELL_ANDROID_UI_APPLICATION_LOADER_ANDROID_H_
