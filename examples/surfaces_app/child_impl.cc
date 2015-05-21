// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/surfaces_app/child_impl.h"

#include "base/bind.h"
#include "cc/output/compositor_frame.h"
#include "cc/output/delegated_frame_data.h"
#include "cc/quads/render_pass.h"
#include "cc/quads/solid_color_draw_quad.h"
#include "examples/surfaces_app/surfaces_util.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/surfaces/surfaces_type_converters.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "ui/gfx/rect.h"
#include "ui/gfx/transform.h"

namespace mojo {
namespace examples {

using cc::RenderPass;
using cc::RenderPassId;
using cc::DrawQuad;
using cc::SolidColorDrawQuad;
using cc::DelegatedFrameData;
using cc::CompositorFrame;

static const uint32_t kLocalId = 1u;

ChildImpl::ChildImpl(ApplicationConnection* surfaces_service_connection,
                     InterfaceRequest<Child> request)
    : id_namespace_(0u), binding_(this, request.Pass()) {
  surfaces_service_connection->ConnectToService(&surface_);
  surface_->GetIdNamespace(
      base::Bind(&ChildImpl::SetIdNamespace, base::Unretained(this)));
  surface_.WaitForIncomingResponse();  // Wait for ID namespace to arrive.
  DCHECK_NE(0u, id_namespace_);
}

ChildImpl::~ChildImpl() {
  surface_->DestroySurface(kLocalId);
}

void ChildImpl::SetIdNamespace(uint32_t id_namespace) {
  id_namespace_ = id_namespace;
}

void ChildImpl::ProduceFrame(ColorPtr color,
                             SizePtr size_ptr,
                             const ProduceCallback& callback) {
  gfx::Size size = size_ptr.To<gfx::Size>();
  gfx::Rect rect(size);
  RenderPassId id(1, 1);
  scoped_ptr<RenderPass> pass = RenderPass::Create();
  pass->SetNew(id, rect, rect, gfx::Transform());

  CreateAndAppendSimpleSharedQuadState(pass.get(), gfx::Transform(), size);

  SolidColorDrawQuad* color_quad =
      pass->CreateAndAppendDrawQuad<SolidColorDrawQuad>();
  bool force_anti_aliasing_off = false;
  color_quad->SetNew(pass->shared_quad_state_list.back(), rect, rect,
                     color.To<SkColor>(), force_anti_aliasing_off);

  scoped_ptr<DelegatedFrameData> delegated_frame_data(new DelegatedFrameData);
  delegated_frame_data->render_pass_list.push_back(pass.Pass());

  scoped_ptr<CompositorFrame> frame(new CompositorFrame);
  frame->delegated_frame_data = delegated_frame_data.Pass();

  surface_->CreateSurface(kLocalId);
  surface_->SubmitFrame(kLocalId, mojo::Frame::From(*frame), mojo::Closure());
  auto qualified_id = mojo::SurfaceId::New();
  qualified_id->id_namespace = id_namespace_;
  qualified_id->local = kLocalId;
  callback.Run(qualified_id.Pass());
}

}  // namespace examples
}  // namespace mojo
