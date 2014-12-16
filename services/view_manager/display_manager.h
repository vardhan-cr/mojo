// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_DISPLAY_MANAGER_H_
#define SERVICES_VIEW_MANAGER_DISPLAY_MANAGER_H_

#include <map>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "cc/surfaces/surface_id.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/services/native_viewport/public/interfaces/native_viewport.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces.mojom.h"
#include "mojo/services/surfaces/public/interfaces/surfaces_service.mojom.h"
#include "ui/gfx/rect.h"

namespace cc {
class SurfaceIdAllocator;
}

namespace mojo {
class ApplicationConnection;
}

namespace view_manager {

class ConnectionManager;
class ServerView;

// DisplayManager is used to connect the root ServerView to a display.
class DisplayManager {
 public:
  virtual ~DisplayManager() {}

  virtual void Init(ConnectionManager* connection_manager) = 0;

  // Schedules a paint for the specified region in the coordinates of |view|.
  virtual void SchedulePaint(const ServerView* view,
                             const gfx::Rect& bounds) = 0;

  virtual void SetViewportSize(const gfx::Size& size) = 0;
};

// DisplayManager implementation that connects to the services necessary to
// actually display.
class DefaultDisplayManager : public DisplayManager,
                              public mojo::NativeViewportClient,
                              public mojo::SurfaceClient {
 public:
  DefaultDisplayManager(
      mojo::ApplicationConnection* app_connection,
      const mojo::Callback<void()>& native_viewport_closed_callback);
  ~DefaultDisplayManager() override;

  // DisplayManager:
  void Init(ConnectionManager* connection_manager) override;
  void SchedulePaint(const ServerView* view, const gfx::Rect& bounds) override;
  void SetViewportSize(const gfx::Size& size) override;

 private:
  void OnCreatedNativeViewport(uint64_t native_viewport_id);
  void OnSurfaceConnectionCreated(mojo::SurfacePtr surface,
                                  uint32_t id_namespace);
  void Draw();

  // NativeViewportClient:
  void OnDestroyed() override;
  void OnSizeChanged(mojo::SizePtr size) override;

  // SurfaceClient:
  void SetIdNamespace(uint32_t id_namespace) override;
  void ReturnResources(
      mojo::Array<mojo::ReturnedResourcePtr> resources) override;

  mojo::ApplicationConnection* app_connection_;
  ConnectionManager* connection_manager_;

  gfx::Size size_;
  gfx::Rect dirty_rect_;
  base::Timer draw_timer_;

  mojo::SurfacesServicePtr surfaces_service_;
  mojo::SurfacePtr surface_;
  scoped_ptr<cc::SurfaceIdAllocator> surface_id_allocator_;
  cc::SurfaceId surface_id_;
  mojo::NativeViewportPtr native_viewport_;
  mojo::Callback<void()> native_viewport_closed_callback_;
  base::WeakPtrFactory<DefaultDisplayManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(DefaultDisplayManager);
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_DISPLAY_MANAGER_H_
