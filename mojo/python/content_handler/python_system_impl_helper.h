// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PYTHON_CONTENT_HANDLER_PYTHON_SYSTEM_IMPL_HELPER_H_
#define MOJO_PYTHON_CONTENT_HANDLER_PYTHON_SYSTEM_IMPL_HELPER_H_

#include <Python.h>

#include "base/base_export.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/public/python/src/common.h"

namespace mojo {
namespace python {

// Run loop for python
class PythonRunLoop {
 public:
  PythonRunLoop();

  ~PythonRunLoop();

  void Run();

  void RunUntilIdle();

  void Quit();

  // Post a task to be executed roughtly after delay microseconds
  void PostDelayedTask(PyObject* callable, int64 delay);
 private:
  base::MessageLoop loop_;
};

PythonAsyncWaiter* NewAsyncWaiter();

}  // namespace python
}  // namespace mojo

#endif  // MOJO_PYTHON_CONTENT_HANDLER_PYTHON_SYSTEM_IMPL_HELPER_H_

