// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/surfaces_app/child_gl_impl.h"

#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES
#endif

#include <GLES2/gl2ext.h>
#include <GLES2/gl2extmojo.h>
#include <MGL/mgl.h>

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/delegated_frame_data.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/texture_draw_quad.h"
#include "examples/surfaces_app/surfaces_util.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/mailbox_holder.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/environment/environment.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace mojo {
namespace examples {

using cc::RenderPass;
using cc::RenderPassId;
using cc::TextureDrawQuad;
using cc::DelegatedFrameData;
using cc::CompositorFrame;

static void ContextLostThunk(void*) {
  LOG(FATAL) << "Context lost";
}

ChildGLImpl::ChildGLImpl(ApplicationConnection* surfaces_service_connection,
                         CommandBufferPtr command_buffer,
                         InterfaceRequest<Child> request)
    : id_namespace_(0u),
      local_id_(1u),
      start_time_(base::TimeTicks::Now()),
      next_resource_id_(1),
      returner_binding_(this),
      binding_(this, request.Pass()) {
  surfaces_service_connection->ConnectToService(&surface_);
  surface_->GetIdNamespace(
      base::Bind(&ChildGLImpl::SetIdNamespace, base::Unretained(this)));
  ResourceReturnerPtr returner_ptr;
  returner_binding_.Bind(GetProxy(&returner_ptr));
  surface_->SetResourceReturner(returner_ptr.Pass());
  context_ = MGLCreateContext(
      MGL_API_VERSION_GLES2,
      command_buffer.PassInterface().PassHandle().release().value(),
      MGL_NO_CONTEXT, &ContextLostThunk, this,
      Environment::GetDefaultAsyncWaiter());
  DCHECK(context_);
  MGLMakeCurrent(context_);
}

ChildGLImpl::~ChildGLImpl() {
  MGLDestroyContext(context_);
  surface_->DestroySurface(local_id_);
}

void ChildGLImpl::ProduceFrame(ColorPtr color,
                               SizePtr size,
                               const ProduceCallback& callback) {
  color_ = color.To<SkColor>();
  size_ = size.To<gfx::Size>();
  cube_.Init();
  cube_.set_size(size_.width(), size_.height());
  cube_.set_color(
      SkColorGetR(color_), SkColorGetG(color_), SkColorGetB(color_));
  surface_->CreateSurface(local_id_);
  produce_callback_ = callback;
  if (id_namespace_ != 0u)
    RunProduceCallback();
  Draw();
}

void ChildGLImpl::SetIdNamespace(uint32_t id_namespace) {
  id_namespace_ = id_namespace;
  if (!produce_callback_.is_null())
    RunProduceCallback();
  produce_callback_.reset();
}

void ChildGLImpl::RunProduceCallback() {
  auto id = SurfaceId::New();
  id->id_namespace = id_namespace_;
  id->local = local_id_;
  produce_callback_.Run(id.Pass());
}

void ChildGLImpl::ReturnResources(Array<ReturnedResourcePtr> resources) {
  for (size_t i = 0; i < resources.size(); ++i) {
    cc::ReturnedResource res = resources[i].To<cc::ReturnedResource>();
    GLuint returned_texture = id_to_tex_map_[res.id];
    glDeleteTextures(1, &returned_texture);
  }
}

void ChildGLImpl::Draw() {
  // First, generate a GL texture and draw the cube into it.
  GLuint texture = 0u;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA,
               size_.width(),
               size_.height(),
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  GLuint fbo = 0u;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  GLuint depth_buffer = 0u;
  glGenRenderbuffers(1, &depth_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, size_.width(),
                        size_.height());
  glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                            GL_RENDERBUFFER, depth_buffer);
  DCHECK_EQ(static_cast<GLenum>(GL_FRAMEBUFFER_COMPLETE),
            glCheckFramebufferStatus(GL_FRAMEBUFFER));
  glClearColor(1, 0, 0, 0.5);
  cube_.UpdateForTimeDelta(0.16f);
  cube_.Draw();
  glDeleteFramebuffers(1, &fbo);
  glDeleteRenderbuffers(1, &depth_buffer);

  // Then, put the texture into a mailbox.
  gpu::Mailbox mailbox = gpu::Mailbox::Generate();
  glProduceTextureCHROMIUM(GL_TEXTURE_2D, mailbox.name);
  GLuint sync_point = glInsertSyncPointCHROMIUM();
  gpu::MailboxHolder holder(mailbox, GL_TEXTURE_2D, sync_point);

  // Then, put the mailbox into a TransferableResource
  cc::TransferableResource resource;
  resource.id = next_resource_id_++;
  id_to_tex_map_[resource.id] = texture;
  resource.format = cc::RGBA_8888;
  resource.filter = GL_LINEAR;
  resource.size = size_;
  resource.mailbox_holder = holder;
  resource.is_repeated = false;
  resource.is_software = false;

  gfx::Rect rect(size_);
  RenderPassId id(1, 1);
  scoped_ptr<RenderPass> pass = RenderPass::Create();
  pass->SetNew(id, rect, rect, gfx::Transform());

  CreateAndAppendSimpleSharedQuadState(pass.get(), gfx::Transform(), size_);

  TextureDrawQuad* texture_quad =
      pass->CreateAndAppendDrawQuad<TextureDrawQuad>();
  float vertex_opacity[4] = {1.0f, 1.0f, 0.2f, 1.0f};
  const bool premultiplied_alpha = true;
  const bool flipped = false;
  const bool nearest_neighbor = false;
  texture_quad->SetNew(pass->shared_quad_state_list.back(),
                       rect,
                       rect,
                       rect,
                       resource.id,
                       premultiplied_alpha,
                       gfx::PointF(),
                       gfx::PointF(1.f, 1.f),
                       SK_ColorBLUE,
                       vertex_opacity,
                       flipped,
                       nearest_neighbor);

  scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
  delegated_frame_data->render_pass_list.push_back(pass.Pass());
  delegated_frame_data->resource_list.push_back(resource);

  scoped_ptr<CompositorFrame> frame(new CompositorFrame);
  frame->delegated_frame_data = delegated_frame_data.Pass();

  surface_->SubmitFrame(local_id_, mojo::Frame::From(*frame), mojo::Closure());

  base::MessageLoop::current()->PostDelayedTask(
      FROM_HERE,
      base::Bind(&ChildGLImpl::Draw, base::Unretained(this)),
      base::TimeDelta::FromMilliseconds(50));
}

}  // namespace examples
}  // namespace mojo
