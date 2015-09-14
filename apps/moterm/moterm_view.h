// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef APPS_MOTERM_MOTERM_VIEW_H_
#define APPS_MOTERM_MOTERM_VIEW_H_

#include "apps/moterm/gl_helper.h"
#include "apps/moterm/moterm_driver.h"
#include "apps/moterm/moterm_model.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/common/binding_set.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/callback.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "mojo/services/surfaces/public/interfaces/surface_id.mojom.h"
#include "mojo/services/terminal/public/interfaces/terminal.mojom.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "skia/ext/refptr.h"
#include "third_party/skia/include/core/SkBitmapDevice.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace mojo {
class Shell;
}

class MotermView : public mojo::ViewObserver,
                   public GlHelper::Client,
                   public MotermModel::Delegate,
                   public MotermDriver::Client,
                   public mojo::InterfaceFactory<mojo::terminal::Terminal>,
                   public mojo::terminal::Terminal {
 public:
  MotermView(
      mojo::Shell* shell,
      mojo::View* view,
      mojo::InterfaceRequest<mojo::ServiceProvider> service_provider_request);
  ~MotermView() override;

 private:
  // |mojo::ViewObserver|:
  void OnViewDestroyed(mojo::View* view) override;
  void OnViewBoundsChanged(mojo::View* view,
                           const mojo::Rect& old_bounds,
                           const mojo::Rect& new_bounds) override;
  void OnViewInputEvent(mojo::View* view, const mojo::EventPtr& event) override;

  // |GlHelper::Client|:
  void OnSurfaceIdChanged(mojo::SurfaceIdPtr surface_id) override;
  void OnContextLost() override;
  void OnFrameDisplayed(uint32_t frame_id) override;

  // |MotermModel::Delegate|:
  void OnResponse(const void* buf, size_t size) override;
  void OnSetKeypadMode(bool application_mode) override;

  // |MotermDriver::Client|:
  void OnDataReceived(const void* bytes, size_t num_bytes) override;
  void OnClosed() override;
  void OnDestroyed() override;

  // |mojo::InterfaceFactory<mojo::terminal::Terminal>|:
  void Create(
      mojo::ApplicationConnection* connection,
      mojo::InterfaceRequest<mojo::terminal::Terminal> request) override;

  // |mojo::terminal::Terminal| implementation:
  void Connect(mojo::InterfaceRequest<mojo::files::File> terminal_file,
               bool force,
               const ConnectCallback& callback) override;
  void ConnectToClient(mojo::terminal::TerminalClientPtr terminal_client,
                       bool force,
                       const ConnectToClientCallback& callback) override;
  void GetSize(const GetSizeCallback& callback) override;
  void SetSize(uint32_t rows,
               uint32_t columns,
               bool reset,
               const SetSizeCallback& callback) override;

  // If |force| is true, it will draw everything. Otherwise it will draw only if
  // |model_state_changes_| is dirty.
  void Draw(bool force);

  void OnKeyPressed(const mojo::EventPtr& key_event);

  mojo::View* const view_;
  GlHelper gl_helper_;

  // TODO(vtl): Consider the structure of this app. Do we really want the "view"
  // owning the model?
  // The terminal model.
  MotermModel model_;
  // State changes to the model since last draw.
  MotermModel::StateChanges model_state_changes_;

  base::WeakPtr<MotermDriver> driver_;
  // If set, called when we get |OnClosed()| or |OnDestroyed()| from the driver.
  mojo::Closure on_closed_callback_;

  mojo::ServiceProviderImpl service_provider_impl_;
  mojo::BindingSet<mojo::terminal::Terminal> terminal_bindings_;

  // TODO(vtl): For some reason, drawing while a frame is already pending (i.e.,
  // we've submitted it but haven't gotten a callback) interacts badly with
  // resizing -- sometimes this results in us losing all future
  // |OnViewBoundsChanged()| messages. So, for now, don't submit frames in that
  // case.
  bool frame_pending_;
  // If we skip drawing despite being forced to, we should force the next draw.
  bool force_next_draw_;

  skia::RefPtr<SkTypeface> regular_typeface_;

  int ascent_;
  int line_height_;
  int advance_width_;

  skia::RefPtr<SkBitmapDevice> bitmap_device_;

  // Keyboard state.
  bool keypad_application_mode_;

  DISALLOW_COPY_AND_ASSIGN(MotermView);
};

#endif  // APPS_MOTERM_MOTERM_VIEW_H_
