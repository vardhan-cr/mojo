// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/window_manager/window_manager_root.h"

#include <algorithm>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "base/message_loop/message_loop.h"
#include "base/stl_util.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "services/window_manager/capture_controller.h"
#include "services/window_manager/focus_controller.h"
#include "services/window_manager/focus_rules.h"
#include "services/window_manager/hit_test.h"
#include "services/window_manager/view_event_dispatcher.h"
#include "services/window_manager/view_target.h"
#include "services/window_manager/view_targeter.h"
#include "services/window_manager/window_manager_delegate.h"

using mojo::ApplicationConnection;
using mojo::Id;
using mojo::ServiceProvider;
using mojo::View;
using mojo::WindowManager;

namespace window_manager {

namespace {

Id GetIdForView(View* view) {
  return view ? view->id() : 0;
}

}  // namespace

// Used for calls to Embed() that occur before we've connected to the
// ViewManager.
struct WindowManagerRoot::PendingEmbed {
  mojo::String url;
  mojo::InterfaceRequest<ServiceProvider> services;
  mojo::ServiceProviderPtr exposed_services;
};

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, public:

WindowManagerRoot::WindowManagerRoot(
    mojo::ApplicationImpl* application_impl,
    mojo::ApplicationConnection* connection,
    WindowManagerControllerFactory* controller_factory,
    mojo::InterfaceRequest<mojo::WindowManager> request)
    : application_impl_(application_impl),
      connection_(connection),
      root_(nullptr) {
  window_manager_controller_ =
      controller_factory->CreateWindowManagerController(connection, this);
  LaunchViewManager(application_impl_);

  // We own WindowManagerImpl. When binding to the request, WindowManagerImpl
  // registers an error callback to WindowManagerRoot::RemoveConnectedService,
  // which will delete it.
  AddConnectedService(make_scoped_ptr(
      new WindowManagerImpl(this, request.PassMessagePipe(), false)));
}

WindowManagerRoot::~WindowManagerRoot() {
  // TODO(msw|sky): Should this destructor explicitly delete the ViewManager?
  mojo::ViewManager* cached_view_manager = view_manager();
  for (RegisteredViewIdSet::const_iterator it = registered_view_id_set_.begin();
       cached_view_manager && it != registered_view_id_set_.end(); ++it) {
    View* view = cached_view_manager->GetViewById(*it);
    if (view && view == root_)
      root_ = nullptr;
    if (view)
      view->RemoveObserver(this);
  }
  registered_view_id_set_.clear();
  DCHECK(!root_);

  // connected_services_ destructor will ensure we don't leak any
  // WindowManagerImpl objects and close all connections.
}

void WindowManagerRoot::AddConnectedService(
    scoped_ptr<WindowManagerImpl> connection) {
  connected_services_.push_back(connection.Pass());
}

void WindowManagerRoot::RemoveConnectedService(WindowManagerImpl* connection) {
  ConnectedServices::iterator position = std::find(
      connected_services_.begin(), connected_services_.end(), connection);
  DCHECK(position != connected_services_.end());

  connected_services_.erase(position);

  // Having no inbound connection is not a guarantee we are no longer used: we
  // can be processing an embed request (App 1 asked for a WindowManager, calls
  // Embed then immediately closes the connection, EmbeddedApp is not yet
  // started).
}

bool WindowManagerRoot::SetCapture(Id view_id) {
  View* view = view_manager()->GetViewById(view_id);
  return view && SetCaptureImpl(view);
}

bool WindowManagerRoot::FocusWindow(Id view_id) {
  View* view = view_manager()->GetViewById(view_id);
  return view && FocusWindowImpl(view);
}

bool WindowManagerRoot::ActivateWindow(Id view_id) {
  View* view = view_manager()->GetViewById(view_id);
  return view && ActivateWindowImpl(view);
}

bool WindowManagerRoot::IsReady() const {
  return root_;
}

void WindowManagerRoot::InitFocus(scoped_ptr<FocusRules> rules) {
  DCHECK(root_);

  focus_controller_.reset(new FocusController(rules.Pass()));
  focus_controller_->AddObserver(this);
  SetFocusController(root_, focus_controller_.get());

  capture_controller_.reset(new CaptureController);
  capture_controller_->AddObserver(this);
  SetCaptureController(root_, capture_controller_.get());
}

void WindowManagerRoot::Embed(
    const mojo::String& url,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  if (view_manager()) {
    window_manager_controller_->Embed(url, services.Pass(),
                                      exposed_services.Pass());
    return;
  }
  scoped_ptr<PendingEmbed> pending_embed(new PendingEmbed);
  pending_embed->url = url;
  pending_embed->services = services.Pass();
  pending_embed->exposed_services = exposed_services.Pass();
  pending_embeds_.push_back(pending_embed.release());
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, ViewManagerDelegate implementation:

void WindowManagerRoot::OnEmbed(
    View* root,
    mojo::InterfaceRequest<mojo::ServiceProvider> services,
    mojo::ServiceProviderPtr exposed_services) {
  DCHECK(!root_);
  root_ = root;

  view_event_dispatcher_.reset(new ViewEventDispatcher);

  RegisterSubtree(root_);

  if (window_manager_controller_.get()) {
    window_manager_controller_->OnEmbed(root, services.Pass(),
                                        exposed_services.Pass());
  }

  for (PendingEmbed* pending_embed : pending_embeds_) {
    Embed(pending_embed->url, pending_embed->services.Pass(),
          pending_embed->exposed_services.Pass());
  }
  pending_embeds_.clear();
}

void WindowManagerRoot::OnViewManagerDisconnected(
    mojo::ViewManager* view_manager) {
  if (window_manager_controller_.get())
    window_manager_controller_->OnViewManagerDisconnected(view_manager);
  delete this;
}

bool WindowManagerRoot::OnPerformAction(mojo::View* view,
                                        const std::string& action) {
  if (!view)
    return false;
  if (action == "capture")
    return SetCaptureImpl(view);
  if (action == "focus")
    return FocusWindowImpl(view);
  else if (action == "activate")
    return ActivateWindowImpl(view);
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, ViewObserver implementation:

void WindowManagerRoot::OnTreeChanged(
    const ViewObserver::TreeChangeParams& params) {
  if (params.receiver != root_)
    return;
  DCHECK(params.old_parent || params.new_parent);
  if (!params.target)
    return;

  if (params.new_parent) {
    if (registered_view_id_set_.find(params.target->id()) ==
        registered_view_id_set_.end()) {
      RegisteredViewIdSet::const_iterator it =
          registered_view_id_set_.find(params.new_parent->id());
      DCHECK(it != registered_view_id_set_.end());
      RegisterSubtree(params.target);
    }
  } else if (params.old_parent) {
    UnregisterSubtree(params.target);
  }
}

void WindowManagerRoot::OnViewDestroying(View* view) {
  Unregister(view);
  if (view == root_) {
    root_ = nullptr;
    if (focus_controller_)
      focus_controller_->RemoveObserver(this);
    if (capture_controller_)
      capture_controller_->RemoveObserver(this);
  }
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, ui::EventHandler implementation:

void WindowManagerRoot::OnEvent(ui::Event* event) {
  if (!window_manager_client_)
    return;

  View* view = static_cast<ViewTarget*>(event->target())->view();
  if (!view)
    return;

  if (event->IsKeyEvent()) {
    const ui::KeyEvent* key_event = static_cast<const ui::KeyEvent*>(event);
    if (key_event->type() == ui::ET_KEY_PRESSED) {
      ui::Accelerator accelerator = ConvertEventToAccelerator(key_event);
      if (accelerator_manager_.Process(accelerator, view))
        return;
    }
  }

  if (focus_controller_)
    focus_controller_->OnEvent(event);

  window_manager_client_->DispatchInputEventToView(view->id(),
                                                   mojo::Event::From(*event));
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, mojo::FocusControllerObserver implementation:

void WindowManagerRoot::OnFocused(View* gained_focus) {
  for (ConnectedServices::const_iterator it = connected_services_.begin();
       it != connected_services_.end(); ++it) {
    (*it)->NotifyViewFocused(GetIdForView(gained_focus));
  }
}

void WindowManagerRoot::OnActivated(View* gained_active) {
  for (ConnectedServices::const_iterator it = connected_services_.begin();
       it != connected_services_.end(); ++it) {
    (*it)->NotifyWindowActivated(GetIdForView(gained_active));
  }
  if (gained_active)
    gained_active->MoveToFront();
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, mojo::CaptureControllerObserver implementation:

void WindowManagerRoot::OnCaptureChanged(View* gained_capture) {
  for (ConnectedServices::const_iterator it = connected_services_.begin();
       it != connected_services_.end(); ++it) {
    (*it)->NotifyCaptureChanged(GetIdForView(gained_capture));
  }
  if (gained_capture)
    gained_capture->MoveToFront();
}

////////////////////////////////////////////////////////////////////////////////
// WindowManagerRoot, private:

bool WindowManagerRoot::SetCaptureImpl(View* view) {
  CHECK(view);
  capture_controller_->SetCapture(view);
  return capture_controller_->GetCapture() == view;
}

bool WindowManagerRoot::FocusWindowImpl(View* view) {
  CHECK(view);
  focus_controller_->FocusView(view);
  return focus_controller_->GetFocusedView() == view;
}

bool WindowManagerRoot::ActivateWindowImpl(View* view) {
  CHECK(view);
  focus_controller_->ActivateView(view);
  return focus_controller_->GetActiveView() == view;
}

void WindowManagerRoot::RegisterSubtree(View* view) {
  view->AddObserver(this);
  DCHECK(registered_view_id_set_.find(view->id()) ==
         registered_view_id_set_.end());
  // All events pass through the root during dispatch, so we only need a handler
  // installed there.
  if (view == root_) {
    ViewTarget* target = ViewTarget::TargetFromView(view);
    target->SetEventTargeter(scoped_ptr<ViewTargeter>(new ViewTargeter()));
    target->AddPreTargetHandler(this);
    view_event_dispatcher_->SetRootViewTarget(target);
  }
  registered_view_id_set_.insert(view->id());
  View::Children::const_iterator it = view->children().begin();
  for (; it != view->children().end(); ++it)
    RegisterSubtree(*it);
}

void WindowManagerRoot::UnregisterSubtree(View* view) {
  for (View* child : view->children())
    UnregisterSubtree(child);
  Unregister(view);
}

void WindowManagerRoot::Unregister(View* view) {
  RegisteredViewIdSet::iterator it = registered_view_id_set_.find(view->id());
  if (it == registered_view_id_set_.end()) {
    // Because we unregister in OnViewDestroying() we can still get a subsequent
    // OnTreeChanged for the same view. Ignore this one.
    return;
  }
  view->RemoveObserver(this);
  DCHECK(it != registered_view_id_set_.end());
  registered_view_id_set_.erase(it);
}

void WindowManagerRoot::DispatchInputEventToView(View* view,
                                                 mojo::EventPtr event) {
  window_manager_client_->DispatchInputEventToView(view->id(), event.Pass());
}

void WindowManagerRoot::SetViewportSize(const gfx::Size& size) {
  window_manager_client_->SetViewportSize(mojo::Size::From(size));
}

void WindowManagerRoot::LaunchViewManager(mojo::ApplicationImpl* app) {
  // TODO(sky): figure out logic if this connection goes away.
  view_manager_client_factory_.reset(
      new mojo::ViewManagerClientFactory(application_impl_->shell(), this));

  ApplicationConnection* view_manager_app =
      app->ConnectToApplication("mojo:view_manager");
  view_manager_app->ConnectToService(&view_manager_service_);

  view_manager_app->AddService<WindowManagerInternal>(this);
  view_manager_app->AddService<mojo::NativeViewportEventDispatcher>(this);

  view_manager_app->ConnectToService(&window_manager_client_);
}

void WindowManagerRoot::Create(
    ApplicationConnection* connection,
    mojo::InterfaceRequest<WindowManagerInternal> request) {
  if (wm_internal_binding_.get()) {
    VLOG(1)
        << "WindowManager allows only one WindowManagerInternal connection.";
    return;
  }
  wm_internal_binding_.reset(
      new mojo::Binding<WindowManagerInternal>(this, request.Pass()));
}

void WindowManagerRoot::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<mojo::NativeViewportEventDispatcher> request) {
  new NativeViewportEventDispatcherImpl(this, request.Pass());
}

void WindowManagerRoot::CreateWindowManagerForViewManagerClient(
    uint16_t connection_id,
    mojo::ScopedMessagePipeHandle window_manager_pipe) {
  // TODO(sky): pass in |connection_id| for validation.
  AddConnectedService(make_scoped_ptr(
      new WindowManagerImpl(this, window_manager_pipe.Pass(), true)));
}

void WindowManagerRoot::SetViewManagerClient(
    mojo::ScopedMessagePipeHandle view_manager_client_request) {
  view_manager_client_.reset(
      mojo::ViewManagerClientFactory::WeakBindViewManagerToPipe(
          mojo::MakeRequest<mojo::ViewManagerClient>(
              view_manager_client_request.Pass()),
          view_manager_service_.Pass(), application_impl_->shell(), this));
}

}  // namespace window_manager
