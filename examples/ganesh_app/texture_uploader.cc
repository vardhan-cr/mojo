// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/ganesh_app/texture_uploader.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2.h>
#include <GLES2/gl2extmojo.h>

#include "base/bind.h"
#include "base/trace_event/trace_event.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/geometry/public/cpp/geometry_util.h"
#include "mojo/services/surfaces/public/cpp/surfaces_utils.h"

namespace examples {

TextureUploader::Client::~Client() {}

TextureUploader::TextureUploader(Client* client,
                                 mojo::Shell* shell,
                                 base::WeakPtr<mojo::GLContext> context)
    : client_(client),
      context_(context),
      next_resource_id_(0u),
      id_namespace_(0u),
      local_id_(0u),
      returner_binding_(this) {
  TRACE_EVENT0("ganesh_app", __func__);
  context_->AddObserver(this);

  mojo::ServiceProviderPtr surfaces_service_provider;
  shell->ConnectToApplication("mojo:surfaces_service",
                              mojo::GetProxy(&surfaces_service_provider),
                              nullptr);
  mojo::ConnectToService(surfaces_service_provider.get(), &surface_);
  surface_->GetIdNamespace(
      base::Bind(&TextureUploader::SetIdNamespace, base::Unretained(this)));
  mojo::ResourceReturnerPtr returner_ptr;
  returner_binding_.Bind(GetProxy(&returner_ptr));
  surface_->SetResourceReturner(returner_ptr.Pass());
}

TextureUploader::~TextureUploader() {
  if (context_.get())
    context_->RemoveObserver(this);
}

void TextureUploader::Upload(scoped_ptr<mojo::GLTexture> texture) {
  TRACE_EVENT0("ganesh_app", __func__);
  mojo::Size size = texture->size();
  EnsureSurfaceForSize(size);

  mojo::FramePtr frame = mojo::Frame::New();
  frame->resources.resize(0u);

  mojo::Rect bounds;
  bounds.width = size.width;
  bounds.height = size.height;
  mojo::PassPtr pass = mojo::CreateDefaultPass(1, bounds);
  pass->quads.resize(0u);
  pass->shared_quad_states.push_back(mojo::CreateDefaultSQS(size));

  context_->MakeCurrent();
  glBindTexture(GL_TEXTURE_2D, texture->texture_id());
  GLbyte mailbox[GL_MAILBOX_SIZE_CHROMIUM];
  glGenMailboxCHROMIUM(mailbox);
  glProduceTextureCHROMIUM(GL_TEXTURE_2D, mailbox);
  GLuint sync_point = glInsertSyncPointCHROMIUM();

  mojo::TransferableResourcePtr resource = mojo::TransferableResource::New();
  resource->id = next_resource_id_++;
  resource_to_texture_map_[resource->id] = texture.release();
  resource->format = mojo::RESOURCE_FORMAT_RGBA_8888;
  resource->filter = GL_LINEAR;
  resource->size = size.Clone();
  mojo::MailboxHolderPtr mailbox_holder = mojo::MailboxHolder::New();
  mailbox_holder->mailbox = mojo::Mailbox::New();
  for (int i = 0; i < GL_MAILBOX_SIZE_CHROMIUM; ++i)
    mailbox_holder->mailbox->name.push_back(mailbox[i]);
  mailbox_holder->texture_target = GL_TEXTURE_2D;
  mailbox_holder->sync_point = sync_point;
  resource->mailbox_holder = mailbox_holder.Pass();
  resource->is_repeated = false;
  resource->is_software = false;

  mojo::QuadPtr quad = mojo::Quad::New();
  quad->material = mojo::MATERIAL_TEXTURE_CONTENT;

  mojo::RectPtr rect = mojo::Rect::New();
  rect->width = size.width;
  rect->height = size.height;
  quad->rect = rect.Clone();
  quad->opaque_rect = rect.Clone();
  quad->visible_rect = rect.Clone();
  quad->needs_blending = true;
  quad->shared_quad_state_index = 0u;

  mojo::TextureQuadStatePtr texture_state = mojo::TextureQuadState::New();
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
  texture_state->flipped = false;

  frame->resources.push_back(resource.Pass());
  quad->texture_quad_state = texture_state.Pass();
  pass->quads.push_back(quad.Pass());

  frame->passes.push_back(pass.Pass());
  surface_->SubmitFrame(local_id_, frame.Pass(), mojo::Closure());
}

void TextureUploader::OnContextLost() {
  LOG(FATAL) << "Context lost.";
}

void TextureUploader::ReturnResources(
    mojo::Array<mojo::ReturnedResourcePtr> resources) {
  TRACE_EVENT0("ganesh_app", __func__);
  context_->MakeCurrent();
  for (size_t i = 0u; i < resources.size(); ++i) {
    mojo::ReturnedResourcePtr resource = resources[i].Pass();
    DCHECK_EQ(1, resource->count);
    glWaitSyncPointCHROMIUM(resource->sync_point);
    mojo::GLTexture* texture = resource_to_texture_map_[resource->id];
    DCHECK_NE(0u, texture->texture_id());
    resource_to_texture_map_.erase(resource->id);
    delete texture;
  }
}

void TextureUploader::EnsureSurfaceForSize(const mojo::Size& size) {
  TRACE_EVENT0("ganesh_app", __func__);
  if (local_id_ != 0u && size == surface_size_)
    return;

  if (local_id_ != 0u) {
    surface_->DestroySurface(local_id_);
  }

  local_id_++;
  surface_->CreateSurface(local_id_);
  surface_size_ = size;
  if (id_namespace_ != 0u)
    SendFullyQualifiedID();
}
void TextureUploader::SendFullyQualifiedID() {
  auto qualified_id = mojo::SurfaceId::New();
  qualified_id->id_namespace = id_namespace_;
  qualified_id->local = local_id_;
  client_->OnSurfaceIdAvailable(qualified_id.Pass());
}

void TextureUploader::SetIdNamespace(uint32_t id_namespace) {
  id_namespace_ = id_namespace;
  if (local_id_ != 0u)
    SendFullyQualifiedID();
}

}  // namespace examples
