# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# distutils: language = c++

from libc.stdint cimport int64_t

cimport c_async_waiter

cdef extern from "mojo/python/content_handler/python_system_impl_helper.h" \
    namespace "mojo::python" nogil:
  cdef cppclass PythonRunLoop "mojo::python::PythonRunLoop":
    PythonRunLoop()
    void Run()
    void RunUntilIdle()
    void Quit()
    void PostDelayedTask(object, int64_t)
  cdef c_async_waiter.PythonAsyncWaiter* NewAsyncWaiter()


