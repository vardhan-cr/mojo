// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/scoped_nsautorelease_pool.h"
#include "ui/gl/gl_surface_ios.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_enums.h"
#include "base/logging.h"

#import <OpenGLES/ES2/gl.h>
#import <QuartzCore/CAEAGLLayer.h>

namespace gfx {

#define WIDGET_AS_LAYER (reinterpret_cast<CAEAGLLayer*>(widget_))
#define CAST_CONTEXT(c) (reinterpret_cast<EAGLContext*>((c)))

GLSurfaceIOS::GLSurfaceIOS(gfx::AcceleratedWidget widget,
                     const gfx::SurfaceConfiguration requested_configuration)
    : GLSurface(requested_configuration),
      widget_(widget),
      framebuffer_(GL_NONE),
      colorbuffer_(GL_NONE),
      last_configured_size_(),
      framebuffer_setup_complete_(false) {
}

#ifndef NDEBUG
static void GLSurfaceIOS_AssertFramebufferCompleteness(void) {
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  DLOG_IF(FATAL, status != GL_FRAMEBUFFER_COMPLETE)
      << "Framebuffer incomplete on GLSurfaceIOS::MakeCurrent: "
      << GLEnums::GetStringEnum(status);
}
#else
#define GLSurfaceIOS_AssertFramebufferCompleteness(...)
#endif

bool GLSurfaceIOS::OnMakeCurrent(GLContext* context) {
  Size new_size = GetSize();

  if (new_size == last_configured_size_) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    GLSurfaceIOS_AssertFramebufferCompleteness();
    return true;
  }

  base::mac::ScopedNSAutoreleasePool pool;

  SetupFramebufferIfNecessary();
  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);

  glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer_);
  DCHECK(glGetError() == GL_NO_ERROR);

  auto context_handle = context->GetHandle();
  DCHECK(context_handle);

  BOOL res = [CAST_CONTEXT(context_handle) renderbufferStorage:GL_RENDERBUFFER
                                                  fromDrawable:WIDGET_AS_LAYER];

  if (!res) {
    return false;
  }

  last_configured_size_ = new_size;
  GLSurfaceIOS_AssertFramebufferCompleteness();
  return true;
}

void GLSurfaceIOS::SetupFramebufferIfNecessary() {
  if (framebuffer_setup_complete_) {
    return;
  }

  DCHECK(framebuffer_ == GL_NONE);
  DCHECK(colorbuffer_ == GL_NONE);

  DCHECK(widget_ != kNullAcceleratedWidget);
  DCHECK(glGetError() == GL_NO_ERROR);

  glGenFramebuffers(1, &framebuffer_);
  DCHECK(framebuffer_ != GL_NONE);

  glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
  DCHECK(glGetError() == GL_NO_ERROR);

  glGenRenderbuffers(1, &colorbuffer_);
  DCHECK(colorbuffer_ != GL_NONE);

  glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer_);
  DCHECK(glGetError() == GL_NO_ERROR);

  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_RENDERBUFFER, colorbuffer_);
  DCHECK(glGetError() == GL_NO_ERROR);

  // TODO(csg): Make use of the surface configuration to select the properties
  // and attachments
  WIDGET_AS_LAYER.drawableProperties = @{
    kEAGLDrawablePropertyColorFormat : kEAGLColorFormatRGBA8,
    kEAGLDrawablePropertyRetainedBacking : @(NO),
  };

  framebuffer_setup_complete_ = true;
}

bool GLSurfaceIOS::SwapBuffers() {
  glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer_);
  return [[EAGLContext currentContext] presentRenderbuffer:GL_RENDERBUFFER];
}

void GLSurfaceIOS::Destroy() {
  DCHECK(glGetError() == GL_NO_ERROR);

  glDeleteFramebuffers(1, &framebuffer_);
  glDeleteRenderbuffers(1, &colorbuffer_);

  DCHECK(glGetError() == GL_NO_ERROR);
}

bool GLSurfaceIOS::IsOffscreen() {
  return widget_ == kNullAcceleratedWidget;
}

gfx::Size GLSurfaceIOS::GetSize() {
  CGSize layer_size = WIDGET_AS_LAYER.bounds.size;
  return Size(layer_size.width, layer_size.height);
}

void* GLSurfaceIOS::GetHandle() {
  return (void*)widget_;
}

bool GLSurface::InitializeOneOffInternal() {
  // On EGL, this method is used to perfom one-time initialization tasks like
  // initializing the display, setting up config lists, etc. There is no such
  // setup on iOS.
  return true;
}

// static
scoped_refptr<GLSurface> GLSurface::CreateViewGLSurface(
      gfx::AcceleratedWidget window,
      const gfx::SurfaceConfiguration& requested_configuration) {
  DCHECK(window != kNullAcceleratedWidget);
  scoped_refptr<GLSurfaceIOS> surface =
    new GLSurfaceIOS(window, requested_configuration);

  if (!surface->Initialize())
    return NULL;

  return surface;
}

}  // namespace gfx
