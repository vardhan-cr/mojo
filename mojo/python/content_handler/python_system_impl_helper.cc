// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "python_system_impl_helper.h"

#include <Python.h>

#include "base/bind.h"
#include "base/location.h"
#include "mojo/common/message_pump_mojo.h"
#include "mojo/public/python/src/common.h"

namespace {
class QuitCurrentRunLoop : public mojo::Closure::Runnable {
 public:
  void Run() const override {
    base::MessageLoop::current()->Quit();
  }

  static mojo::Closure::Runnable* NewInstance() {
    return new QuitCurrentRunLoop();
  }
};

}  // namespace
namespace mojo {
namespace python {

PythonRunLoop::PythonRunLoop() : loop_(common::MessagePumpMojo::Create()) {
}

PythonRunLoop::~PythonRunLoop() {
}

void PythonRunLoop::Run() {
  loop_.Run();
}

void PythonRunLoop::RunUntilIdle() {
  loop_.RunUntilIdle();
}

void PythonRunLoop::Quit() {
  loop_.Quit();
}

void PythonRunLoop::PostDelayedTask(PyObject* callable, MojoTimeTicks delay) {
  Closure::Runnable* quit_runnable =
      NewRunnableFromCallable(callable, loop_.QuitClosure());

  loop_.PostDelayedTask(
      FROM_HERE, base::Bind(&mojo::Closure::Run,
                            base::Owned(new mojo::Closure(quit_runnable))),
      base::TimeDelta::FromMicroseconds(delay));
}

PythonAsyncWaiter* NewAsyncWaiter() {
  return new PythonAsyncWaiter(
      mojo::Closure(QuitCurrentRunLoop::NewInstance()));
}

}  // namespace python
}  // namespace mojo
