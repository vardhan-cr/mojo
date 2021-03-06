// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_
#define SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "shell/android/android_handler.h"
#include "shell/application_manager/application_loader.h"

namespace shell {

class AndroidHandlerLoader : public ApplicationLoader {
 public:
  AndroidHandlerLoader();
  virtual ~AndroidHandlerLoader();

 private:
  // ApplicationLoader overrides:
  void Load(
      const GURL& url,
      mojo::InterfaceRequest<mojo::Application> application_request) override;

  AndroidHandler android_handler_;
  scoped_ptr<mojo::ApplicationImpl> application_;

  DISALLOW_COPY_AND_ASSIGN(AndroidHandlerLoader);
};

}  // namespace shell

#endif  // SHELL_ANDROID_ANDROID_HANDLER_LOADER_H_
