// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/surfaces/context_provider_mojo.h"

#include "base/logging.h"
#include "mojo/gpu/mojo_context_support.h"
#include "mojo/gpu/mojo_gles2_impl_autogen.h"
#include "mojo/public/cpp/environment/environment.h"

namespace mojo {

ContextProviderMojo::ContextProviderMojo(
    ScopedMessagePipeHandle command_buffer_handle)
    : command_buffer_handle_(command_buffer_handle.Pass()),
      context_(nullptr),
      context_lost_(false) {
}

bool ContextProviderMojo::BindToCurrentThread() {
  DCHECK(command_buffer_handle_.is_valid());
  context_ = MojoGLES2CreateContext(command_buffer_handle_.release().value(),
                                    &ContextLostThunk, this,
                                    Environment::GetDefaultAsyncWaiter());
  DCHECK(context_);
  context_support_.reset(new MojoContextSupport(context_));
  gles2_impl_.reset(new MojoGLES2Impl(context_));
  return !!context_;
}

gpu::gles2::GLES2Interface* ContextProviderMojo::ContextGL() {
  return gles2_impl_.get();
}

gpu::ContextSupport* ContextProviderMojo::ContextSupport() {
  return context_support_.get();
}

class GrContext* ContextProviderMojo::GrContext() {
  return NULL;
}

cc::ContextProvider::Capabilities ContextProviderMojo::ContextCapabilities() {
  return capabilities_;
}

void ContextProviderMojo::SetupLock() {
}

base::Lock* ContextProviderMojo::GetLock() {
  return &context_lock_;
}

bool ContextProviderMojo::IsContextLost() {
  return context_lost_;
}
bool ContextProviderMojo::DestroyedOnMainThread() {
  return !context_;
}

void ContextProviderMojo::SetLostContextCallback(
    const LostContextCallback& lost_context_callback) {
  lost_context_callback_ = lost_context_callback;
}

ContextProviderMojo::~ContextProviderMojo() {
  if (context_)
    MojoGLES2DestroyContext(context_);
}

void ContextProviderMojo::ContextLost() {
  context_lost_ = true;
  if (!lost_context_callback_.is_null())
    lost_context_callback_.Run();
}

}  // namespace mojo
