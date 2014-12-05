// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_TEST_CHILD_PROCESS_H_
#define SHELL_TEST_CHILD_PROCESS_H_

#include "base/macros.h"
#include "shell/child_process.h"

namespace mojo {
namespace shell {

class TestChildProcess : public ChildProcess {
 public:
  TestChildProcess();
  ~TestChildProcess() override;

  void Main() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestChildProcess);
};

}  // namespace shell
}  // namespace mojo

#endif  // SHELL_TEST_CHILD_PROCESS_H_
