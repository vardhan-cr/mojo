// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CONTEXT_H_
#define SHELL_CONTEXT_H_

#include <string>

#include "base/macros.h"
#include "shell/application_manager/application_manager.h"
#include "shell/mojo_url_resolver.h"
#include "shell/task_runners.h"

namespace mojo {

namespace shell {

class DynamicApplicationLoader;
class ExternalApplicationListener;

// The "global" context for the shell's main process.
class Context : ApplicationManager::Delegate {
 public:
  Context();
  ~Context() override;

  // Point to the directory containing installed services, such as the network
  // service. By default this directory is used as the base URL for resolving
  // unknown mojo: URLs. The network service will be loaded from this directory,
  // even when the base URL for unknown mojo: URLs is overridden.
  void SetShellFileRoot(const base::FilePath& path);

  // Resolve an URL relative to the shell file root. This is a nop for
  // everything but relative file URLs or URLs without a scheme.
  GURL ResolveShellFileURL(const std::string& path);

  // Override the CWD, which is used for resolving file URLs passed in from the
  // command line.
  void SetCommandLineCWD(const base::FilePath& path);

  // Resolve an URL relative to the CWD mojo_shell was invoked from. This is a
  // nop for everything but relative file URLs or URLs without a scheme.
  GURL ResolveCommandLineURL(const std::string& path);

  static void EnsureEmbedderIsInitialized();
  bool Init();

  void Run(const GURL& url);
  ScopedMessagePipeHandle ConnectToServiceByName(
      const GURL& application_url,
      const std::string& service_name);

  TaskRunners* task_runners() { return task_runners_.get(); }
  ApplicationManager* application_manager() { return &application_manager_; }
  MojoURLResolver* mojo_url_resolver() { return &mojo_url_resolver_; }

 private:
  class NativeViewportApplicationLoader;

  // ApplicationManager::Delegate override.
  void OnApplicationError(const GURL& url) override;
  GURL ResolveURL(const GURL& url) override;
  GURL ResolveMappings(const GURL& url) override;

  std::set<GURL> app_urls_;
  scoped_ptr<TaskRunners> task_runners_;
  scoped_ptr<ExternalApplicationListener> listener_;
  ApplicationManager application_manager_;
  MojoURLResolver mojo_url_resolver_;
  GURL shell_file_root_;
  GURL command_line_cwd_;

  DISALLOW_COPY_AND_ASSIGN(Context);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_CONTEXT_H_
