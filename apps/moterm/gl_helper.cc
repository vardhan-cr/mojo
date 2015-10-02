// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "apps/moterm/gl_helper.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>
#include <MGL/mgl.h>

#include "base/bind.h"
#include "base/location.h"
#include "base/message_loop/message_loop.h"
#include "base/task_runner.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/geometry/public/cpp/geometry_util.h"
#include "mojo/services/surfaces/public/cpp/surfaces_utils.h"

// Maximum number of (live) textures to keep around.
const size_t kMaxTextures = 10;

GlHelper::GlHelper(Client* client,
                   mojo::Shell* shell,
                   GLint texture_format,
                   bool flipped,
                   const mojo::Size& initial_size)
    : client_(client),
      texture_format_(texture_format),
      flipped_(flipped),
      returner_binding_(this),
      next_surface_size_(initial_size),
      mgl_context_(MGL_NO_CONTEXT),
      next_frame_id_(0),
      frame_texture_(0),
      id_namespace_(0),
      local_id_(0),
      next_resource_id_(0),
      weak_factory_(this) {
  mojo::ServiceProviderPtr native_viewport_service_provider;
  shell->ConnectToApplication("mojo:native_viewport_service",
                              GetProxy(&native_viewport_service_provider),
                              nullptr);
  mojo::ConnectToService(native_viewport_service_provider.get(), &gpu_);

  mojo::ServiceProviderPtr surfaces_service_provider;
  shell->ConnectToApplication("mojo:surfaces_service",
                              GetProxy(&surfaces_service_provider), nullptr);
  mojo::ConnectToService(surfaces_service_provider.get(), &surface_);
  surface_->GetIdNamespace(base::Bind(&GlHelper::GetIdNamespaceCallback,
                                      weak_factory_.GetWeakPtr()));
  mojo::ResourceReturnerPtr returner_ptr;
  returner_binding_.Bind(GetProxy(&returner_ptr));
  surface_->SetResourceReturner(returner_ptr.Pass());
}

GlHelper::~GlHelper() {
  DCHECK(!frame_texture_);
  if (mgl_context_ != MGL_NO_CONTEXT)
    MGLDestroyContext(mgl_context_);
}

void GlHelper::SetSurfaceSize(const mojo::Size& surface_size) {
  next_surface_size_ = surface_size;
}

void GlHelper::MakeCurrent() {
  EnsureContext();
}

void GlHelper::StartFrame() {
  DCHECK(!frame_texture_);

  EnsureContext();
  EnsureSurface();

  TextureInfo texture_info = GetTexture();
  DCHECK(texture_info.texture);
  frame_texture_ = texture_info.texture;

  // It's already bound.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

uint32_t GlHelper::EndFrame() {
  DCHECK(frame_texture_);

  mojo::Rect size_rect;
  size_rect.width = current_surface_size_.width;
  size_rect.height = current_surface_size_.height;

  GLbyte mailbox[GL_MAILBOX_SIZE_CHROMIUM];
  glGenMailboxCHROMIUM(mailbox);
  glProduceTextureCHROMIUM(GL_TEXTURE_2D, mailbox);
  GLuint sync_point = glInsertSyncPointCHROMIUM();

  mojo::FramePtr frame = mojo::Frame::New();

  // Frame resources:
  frame->resources.push_back(mojo::TransferableResource::New());
  mojo::TransferableResource* resource = frame->resources[0].get();
  resource->id = next_resource_id_++;
  textures_pending_return_.push_back(
      TextureInfo(resource->id, frame_texture_, current_surface_size_));
  frame_texture_ = 0;
  // TODO(vtl): This is wrong, but doesn't seem to have an effect.
  resource->format = mojo::ResourceFormat::RGBA_8888;
  resource->filter = GL_LINEAR;
  resource->size = current_surface_size_.Clone();
  mojo::MailboxHolderPtr mailbox_holder = mojo::MailboxHolder::New();
  mailbox_holder->mailbox = mojo::Mailbox::New();
  for (int i = 0; i < GL_MAILBOX_SIZE_CHROMIUM; ++i)
    mailbox_holder->mailbox->name.push_back(mailbox[i]);
  mailbox_holder->texture_target = GL_TEXTURE_2D;
  mailbox_holder->sync_point = sync_point;
  resource->mailbox_holder = mailbox_holder.Pass();
  resource->is_repeated = false;
  resource->is_software = false;

  // Frame passes:
  frame->passes.push_back(mojo::CreateDefaultPass(1, size_rect));
  mojo::Pass* pass = frame->passes[0].get();
  pass->quads.push_back(mojo::Quad::New());
  mojo::Quad* quad = pass->quads[0].get();
  quad->material = mojo::Material::TEXTURE_CONTENT;
  quad->rect = size_rect.Clone();
  quad->opaque_rect = size_rect.Clone();
  quad->visible_rect = size_rect.Clone();
  quad->needs_blending = true;
  quad->shared_quad_state_index = 0;
  quad->texture_quad_state = mojo::TextureQuadState::New();
  mojo::TextureQuadState* texture_state = quad->texture_quad_state.get();
  texture_state->resource_id = resource->id;
  texture_state->premultiplied_alpha = true;
  texture_state->uv_top_left = mojo::PointF::New();
  texture_state->uv_bottom_right = mojo::PointF::New();
  texture_state->uv_bottom_right->x = 1.f;
  texture_state->uv_bottom_right->y = 1.f;
  texture_state->background_color = mojo::Color::New();
  texture_state->background_color->rgba = 0;
  for (int i = 0; i < 4; ++i)
    texture_state->vertex_opacity.push_back(1.f);
  texture_state->flipped = flipped_;
  pass->shared_quad_states.push_back(
      mojo::CreateDefaultSQS(current_surface_size_));

  surface_->SubmitFrame(local_id_, frame.Pass(),
                        base::Bind(&GlHelper::SubmitFrameCallback,
                                   weak_factory_.GetWeakPtr(), next_frame_id_));

  return next_frame_id_++;
}

GLuint GlHelper::GetFrameTexture() {
  DCHECK(frame_texture_);
  return frame_texture_;
}

void GlHelper::ReturnResources(
    mojo::Array<mojo::ReturnedResourcePtr> resources) {
  DCHECK(!frame_texture_);

  if (mgl_context_ == MGL_NO_CONTEXT) {
    DCHECK(textures_pending_return_.empty());
    return;
  }

  MGLMakeCurrent(mgl_context_);

  // Note: This quadratic nested loop is OK, since we expect both |resources|
  // and |textures_pending_return_| to be small (and |resources| should
  // usually have just a single element).
  for (size_t i = 0; i < resources.size(); ++i) {
    mojo::ReturnedResourcePtr resource = resources[i].Pass();
    DCHECK_EQ(resource->count, 1);

    bool found = false;
    for (size_t j = 0; j < textures_pending_return_.size(); j++) {
      const TextureInfo& texture_info = textures_pending_return_[j];
      if (texture_info.resource_id == resource->id) {
        glWaitSyncPointCHROMIUM(resource->sync_point);
        ReturnTexture(texture_info);
        textures_pending_return_.erase(textures_pending_return_.begin() + j);
        found = true;
        break;
      }
    }
    if (!found) {
      // If we don't texture ID for it, assume we lost the context.
      // TODO(vtl): This may leak (but currently we don't know if the texture is
      // still valid).
      DVLOG(1) << "Returned texture not found (context lost?)";
    }
  }
}

void GlHelper::EnsureContext() {
  DCHECK(!frame_texture_);

  if (mgl_context_ == MGL_NO_CONTEXT) {
    DCHECK(textures_pending_return_.empty());

    mojo::CommandBufferPtr command_buffer;
    gpu_->CreateOffscreenGLES2Context(mojo::GetProxy(&command_buffer));
    mgl_context_ = MGLCreateContext(
        MGL_API_VERSION_GLES2,
        command_buffer.PassInterface().PassHandle().release().value(), nullptr,
        &GlHelper::OnContextLostThunk, this,
        mojo::Environment::GetDefaultAsyncWaiter());
    CHECK_NE(mgl_context_, MGL_NO_CONTEXT);
  }

  MGLMakeCurrent(mgl_context_);
}

void GlHelper::EnsureSurface() {
  DCHECK(!frame_texture_);
  DCHECK_NE(mgl_context_, MGL_NO_CONTEXT);

  if (local_id_) {
    if (current_surface_size_ == next_surface_size_)
      return;

    surface_->DestroySurface(local_id_);

    ClearTextures();
  }

  local_id_++;
  surface_->CreateSurface(local_id_);
  current_surface_size_ = next_surface_size_;
  if (id_namespace_) {
    // Don't call the client in the nested context.
    base::MessageLoop::current()->task_runner()->PostTask(
        FROM_HERE, base::Bind(&GlHelper::CallOnSurfaceIdChanged,
                              weak_factory_.GetWeakPtr()));
  }
}

void GlHelper::CallOnSurfaceIdChanged() {
  DCHECK(id_namespace_ && local_id_);

  auto qualified_id = mojo::SurfaceId::New();
  qualified_id->id_namespace = id_namespace_;
  qualified_id->local = local_id_;
  client_->OnSurfaceIdChanged(qualified_id.Pass());
}

GlHelper::TextureInfo GlHelper::GetTexture() {
  DCHECK_NE(mgl_context_, MGL_NO_CONTEXT);

  if (!textures_.empty()) {
    TextureInfo rv = textures_.front();
    DCHECK(rv.size == current_surface_size_);
    textures_.pop_front();
    glBindTexture(GL_TEXTURE_2D, rv.texture);
    return rv;
  }

  GLuint texture = 0;
  glGenTextures(1, &texture);
  DCHECK(texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, texture_format_, current_surface_size_.width,
               current_surface_size_.height, 0, texture_format_,
               GL_UNSIGNED_BYTE, nullptr);

  return TextureInfo(0, texture, current_surface_size_);
}

void GlHelper::ReturnTexture(const TextureInfo& texture_info) {
  DCHECK_NE(mgl_context_, MGL_NO_CONTEXT);
  DCHECK_NE(texture_info.texture, 0u);

  if (texture_info.size == current_surface_size_ &&
      textures_.size() < kMaxTextures)
    textures_.push_back(texture_info);  // TODO(vtl): Is |push_front()| better?
  else
    glDeleteTextures(1, &texture_info.texture);
}

void GlHelper::ClearTextures() {
  DCHECK_NE(mgl_context_, MGL_NO_CONTEXT);

  for (const auto& texture_info : textures_)
    glDeleteTextures(1, &texture_info.texture);

  textures_.clear();
}

void GlHelper::GetIdNamespaceCallback(uint32_t id_namespace) {
  id_namespace_ = id_namespace;
  if (local_id_) {
    // We're in a callback, so we can just call the client directly.
    CallOnSurfaceIdChanged();
  }
}

// static
void GlHelper::OnContextLostThunk(void* self) {
  static_cast<GlHelper*>(self)->OnContextLost();
}

void GlHelper::OnContextLost() {
  // We shouldn't get this while we're processing a frame.
  DCHECK(!frame_texture_);

  DCHECK_NE(mgl_context_, MGL_NO_CONTEXT);
  MGLDestroyContext(mgl_context_);
  mgl_context_ = MGL_NO_CONTEXT;

  // TODO(vtl): We don't know if any of those textures will be valid when
  // returned (if they are), so assume they aren't.
  textures_pending_return_.clear();

  // We're in a callback, so we can just call the client directly.
  client_->OnContextLost();
}

void GlHelper::SubmitFrameCallback(uint32_t frame_id) {
  // We're in a callback, so we can just call the client directly.
  client_->OnFrameDisplayed(frame_id);
}
