// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_CHILD_MAIN_H_
#define SHELL_CHILD_MAIN_H_

namespace mojo {
namespace shell {

// The "main()" for child processes. This should be called with a
// the |base::CommandLine| singleton initialized and a |base::AtExitManager|,
// but without a |base::MessageLoop| for the current thread. Returns the exit
// code for the process.
int ChildMain();

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_CHILD_MAIN_H_
