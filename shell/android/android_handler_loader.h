// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_
#define SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/application_manager/application_loader.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "shell/android/android_handler.h"

namespace mojo {
namespace shell {

class AndroidHandlerLoader : public ApplicationLoader {
 public:
  AndroidHandlerLoader();
  virtual ~AndroidHandlerLoader();

 private:
  // ApplicationLoader overrides:
  void Load(ApplicationManager* manager,
            const GURL& url,
            ScopedMessagePipeHandle shell_handle,
            LoadCallback callback) override;
  void OnApplicationError(ApplicationManager* manager,
                          const GURL& url) override;

  AndroidHandler android_handler_;
  scoped_ptr<ApplicationImpl> application_;

  DISALLOW_COPY_AND_ASSIGN(AndroidHandlerLoader);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_
