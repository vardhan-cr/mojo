// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_GANESH_APP_GANESH_VIEW_H_
#define EXAMPLES_GANESH_APP_GANESH_VIEW_H_

#include "examples/ganesh_app/texture_uploader.h"
#include "mojo/gpu/gl_context.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "mojo/skia/ganesh_context.h"

namespace mojo {
class Shell;
}

namespace examples {

class GaneshView : public TextureUploader::Client, public mojo::ViewObserver {
 public:
  GaneshView(mojo::Shell* shell, mojo::View* view);
  ~GaneshView() override;

 private:
  // mojo::ViewObserver implementation.
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

  // TextureUploader::Client implementation.
  void OnSurfaceIdAvailable(mojo::SurfaceIdPtr surface_id) override;

  void Draw(const mojo::Size& size);

  mojo::View* view_;
  base::WeakPtr<mojo::GLContext> gl_context_;
  scoped_ptr<mojo::GaneshContext> gr_context_;
  TextureUploader texture_uploader_;

  DISALLOW_COPY_AND_ASSIGN(GaneshView);
};

}  // namespace examples

#endif  // EXAMPLES_GANESH_APP_GANESH_VIEW_H_
