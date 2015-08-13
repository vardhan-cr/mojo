// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file implements the MGL and MGL onscreen entry points exposed to the
// Mojo application by the shell.
//
// TODO(jamesr): These entry points are implemented on top of the MojoGLES2
// family, which for control functions is backwards.  The MojoGLES2 control
// entry points should be implemented on top of these entry points while we
// deprecate them.

#include "mojo/public/c/gles2/gles2.h"
#include "mojo/public/c/gpu/MGL/mgl.h"
#include "mojo/public/c/gpu/MGL/mgl_onscreen.h"
#include "mojo/public/cpp/system/message_pipe.h"

extern "C" {

MGLContext MGLCreateContext(MGLOpenGLAPIVersion version,
                            MojoHandle command_buffer_handle,
                            MGLContext share_group,
                            MGLContextLostCallback lost_callback,
                            void* lost_callback_closure,
                            const struct MojoAsyncWaiter* async_waiter) {
  // TODO(jamesr): Support ES 3.0 / 3.1 where possible.
  if (version != MGL_API_VERSION_GLES2) {
    MojoClose(command_buffer_handle);
    return MGL_NO_CONTEXT;
  }
  // TODO(jamesr): Plumb through share groups.
  if (share_group != MGL_NO_CONTEXT) {
    MojoClose(command_buffer_handle);
    return MGL_NO_CONTEXT;
  }

  return reinterpret_cast<MGLContext>(
      MojoGLES2CreateContext(command_buffer_handle, lost_callback,
                             lost_callback_closure, async_waiter));
}

void MGLDestroyContext(MGLContext context) {
  MojoGLES2DestroyContext(reinterpret_cast<MojoGLES2Context>(context));
}

void MGLMakeCurrent(MGLContext context) {
  MojoGLES2MakeCurrent(reinterpret_cast<MojoGLES2Context>(context));
  // TODO(jamesr): Plumb through loss information.
}

MGLContext MGLGetCurrentContext() {
  // TODO(jamesr): Implement.
  return MGL_NO_CONTEXT;
}

// TODO(jamesr): Hack - this symbol is defined in //mojo/gles2/gles2_impl.cc.
// What should really happen here is this should call into the data structure
// underlying MGLContext, but that can't happen until control is inverted.
void MojoGLES2glResizeCHROMIUM(uint32_t width,
                               uint32_t height,
                               float scale_factor);

void MGLResizeSurface(uint32_t width, uint32_t height) {
  // TODO(jamesr): Implement
  MojoGLES2glResizeCHROMIUM(width, height, 1.f);
}

void MGLSwapBuffers() {
  MojoGLES2SwapBuffers();
}

}  // extern "C"
