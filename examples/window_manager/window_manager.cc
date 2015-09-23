// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/macros.h"
#include "examples/window_manager/debug_panel_host.mojom.h"
#include "examples/window_manager/window_manager.mojom.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/binding_set.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/input_events/public/interfaces/input_events.mojom.h"
#include "mojo/services/navigation/public/interfaces/navigation.mojom.h"
#include "mojo/services/view_manager/public/cpp/view.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_manager_delegate.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "services/window_manager/basic_focus_rules.h"
#include "services/window_manager/view_target.h"
#include "services/window_manager/window_manager_app.h"
#include "services/window_manager/window_manager_delegate.h"
#include "services/window_manager/window_manager_root.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "url/gurl.h"

#if defined CreateWindow
#undef CreateWindow
#endif

namespace mojo {
namespace examples {

class WindowManagerController;

namespace {

const int kBorderInset = 25;
const int kControlPanelWidth = 200;
const int kTextfieldHeight = 39;

}  // namespace

class WindowManagerConnection : public ::examples::IWindowManager {
 public:
  WindowManagerConnection(WindowManagerController* window_manager,
                          InterfaceRequest<::examples::IWindowManager> request)
      : window_manager_(window_manager), binding_(this, request.Pass()) {}
  ~WindowManagerConnection() override {}

 private:
  // Overridden from ::examples::IWindowManager:
  void CloseWindow(Id view_id) override;

  WindowManagerController* window_manager_;
  StrongBinding<::examples::IWindowManager> binding_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerConnection);
};

class NavigatorHostImpl : public NavigatorHost {
 public:
  NavigatorHostImpl(WindowManagerController* window_manager, Id view_id)
      : window_manager_(window_manager),
        view_id_(view_id),
        current_index_(-1) {}
  ~NavigatorHostImpl() override {}

  void Bind(InterfaceRequest<NavigatorHost> request) {
    bindings_.AddBinding(this, request.Pass());
  }

  void RecordNavigation(const std::string& url);

 private:
  void DidNavigateLocally(const mojo::String& url) override;
  void RequestNavigate(Target target, URLRequestPtr request) override;
  void RequestNavigateHistory(int32_t delta) override;

  WindowManagerController* window_manager_;
  Id view_id_;
  std::vector<std::string> history_;
  int32_t current_index_;

  BindingSet<NavigatorHost> bindings_;

  DISALLOW_COPY_AND_ASSIGN(NavigatorHostImpl);
};

class RootLayoutManager : public ViewObserver {
 public:
  RootLayoutManager(ViewManager* view_manager,
                    View* root,
                    Id content_view_id,
                    Id launcher_ui_view_id,
                    Id control_panel_view_id)
      : root_(root),
        view_manager_(view_manager),
        content_view_id_(content_view_id),
        launcher_ui_view_id_(launcher_ui_view_id),
        control_panel_view_id_(control_panel_view_id) {}
  ~RootLayoutManager() override {
    if (root_)
      root_->RemoveObserver(this);
  }

 private:
  // Overridden from ViewObserver:
  void OnViewBoundsChanged(View* view,
                           const Rect& old_bounds,
                           const Rect& new_bounds) override {
    DCHECK_EQ(view, root_);

    View* content_view = view_manager_->GetViewById(content_view_id_);
    content_view->SetBounds(new_bounds);

    int delta_width = new_bounds.width - old_bounds.width;
    int delta_height = new_bounds.height - old_bounds.height;

    View* launcher_ui_view = view_manager_->GetViewById(launcher_ui_view_id_);
    Rect launcher_ui_bounds(launcher_ui_view->bounds());
    launcher_ui_bounds.width += delta_width;
    launcher_ui_view->SetBounds(launcher_ui_bounds);

    View* control_panel_view =
        view_manager_->GetViewById(control_panel_view_id_);
    Rect control_panel_bounds(control_panel_view->bounds());
    control_panel_bounds.x += delta_width;
    control_panel_view->SetBounds(control_panel_bounds);

    const View::Children& content_views = content_view->children();
    View::Children::const_iterator iter = content_views.begin();
    for (; iter != content_views.end(); ++iter) {
      View* view = *iter;
      if (view->id() == control_panel_view->id() ||
          view->id() == launcher_ui_view->id())
        continue;
      Rect view_bounds(view->bounds());
      view_bounds.width += delta_width;
      view_bounds.height += delta_height;
      view->SetBounds(view_bounds);
    }
  }
  void OnViewDestroyed(View* view) override {
    DCHECK_EQ(view, root_);
    root_->RemoveObserver(this);
    root_ = NULL;
  }

  View* root_;
  ViewManager* view_manager_;
  const Id content_view_id_;
  const Id launcher_ui_view_id_;
  const Id control_panel_view_id_;

  DISALLOW_COPY_AND_ASSIGN(RootLayoutManager);
};

class Window : public InterfaceFactory<NavigatorHost> {
 public:
  Window(WindowManagerController* window_manager, View* view)
      : window_manager_(window_manager),
        view_(view),
        navigator_host_(window_manager_, view_->id()) {
    exposed_services_impl_.AddService<NavigatorHost>(this);
  }

  ~Window() override {}

  View* view() const { return view_; }

  NavigatorHost* navigator_host() { return &navigator_host_; }

  void Embed(const std::string& url) {
    // TODO: Support embedding multiple times?
    ServiceProviderPtr exposed_services;
    exposed_services_impl_.Bind(GetProxy(&exposed_services));
    view_->Embed(url, nullptr, exposed_services.Pass());
    navigator_host_.RecordNavigation(url);
  }

 private:
  // InterfaceFactory<NavigatorHost>
  void Create(ApplicationConnection* connection,
              InterfaceRequest<NavigatorHost> request) override {
    navigator_host_.Bind(request.Pass());
  }

  WindowManagerController* window_manager_;
  View* view_;
  ServiceProviderImpl exposed_services_impl_;
  NavigatorHostImpl navigator_host_;
};

class WindowManagerController
    : public examples::DebugPanelHost,
      public window_manager::WindowManagerController,
      public ui::EventHandler,
      public ui::AcceleratorTarget,
      public mojo::InterfaceFactory<examples::DebugPanelHost>,
      public InterfaceFactory<::examples::IWindowManager> {
 public:
  WindowManagerController(Shell* shell,
                          ApplicationImpl* app,
                          ApplicationConnection* connection,
                          window_manager::WindowManagerRoot* wm_root)
      : shell_(shell),
        launcher_ui_(NULL),
        view_manager_(NULL),
        window_manager_root_(wm_root),
        navigation_target_(TARGET_DEFAULT),
        app_(app),
        binding_(this) {
    connection->AddService<::examples::IWindowManager>(this);
  }

  ~WindowManagerController() override {
    // host() may be destroyed by the time we get here.
    // TODO: figure out a way to always cleanly remove handler.

    // TODO(erg): In the aura version, we removed ourselves from the
    // PreTargetHandler list here. We may need to do something analogous when
    // we get event handling without aura working.
  }

  void CloseWindow(Id view_id) {
    WindowVector::iterator iter = GetWindowByViewId(view_id);
    DCHECK(iter != windows_.end());
    Window* window = *iter;
    windows_.erase(iter);
    window->view()->Destroy();
  }

  void DidNavigateLocally(uint32 source_view_id, const mojo::String& url) {
    LOG(ERROR) << "DidNavigateLocally: source_view_id: " << source_view_id
               << " url: " << url.To<std::string>();
  }

  void RequestNavigate(uint32 source_view_id,
                       Target target,
                       const mojo::String& url) {
    OnLaunch(source_view_id, target, url);
  }

  // Overridden from mojo::DebugPanelHost:
  void CloseTopWindow() override {
    if (!windows_.empty())
      CloseWindow(windows_.back()->view()->id());
  }

  void NavigateTo(const String& url) override {
    OnLaunch(control_panel_id_, TARGET_NEW_NODE, url);
  }

  void SetNavigationTarget(Target t) override { navigation_target_ = t; }

  // mojo::InterfaceFactory<examples::DebugPanelHost> implementation.
  void Create(
      mojo::ApplicationConnection* connection,
      mojo::InterfaceRequest<examples::DebugPanelHost> request) override {
    binding_.Bind(request.Pass());
  }

  // mojo::InterfaceFactory<::examples::IWindowManager> implementation.
  void Create(
      mojo::ApplicationConnection* connection,
      mojo::InterfaceRequest<::examples::IWindowManager> request) override {
    new WindowManagerConnection(this, request.Pass());
  }

 private:
  typedef std::vector<Window*> WindowVector;

  // Overridden from ViewManagerDelegate:
  void OnEmbed(View* root,
               InterfaceRequest<ServiceProvider> services,
               ServiceProviderPtr exposed_services) override {
    DCHECK(!view_manager_);
    view_manager_ = root->view_manager();

    View* view = view_manager_->CreateView();
    root->AddChild(view);
    Rect rect;
    rect.width = root->bounds().width;
    rect.height = root->bounds().height;
    view->SetBounds(rect);
    view->SetVisible(true);
    content_view_id_ = view->id();

    Id launcher_ui_id = CreateLauncherUI();
    control_panel_id_ = CreateControlPanel(view);

    root_layout_manager_.reset(
        new RootLayoutManager(view_manager_, root, content_view_id_,
                              launcher_ui_id, control_panel_id_));
    root->AddObserver(root_layout_manager_.get());

    // TODO(erg): In the aura version, we explicitly added ourselves as a
    // PreTargetHandler to the window() here. We probably have to do something
    // analogous here.

    window_manager_root_->InitFocus(
        make_scoped_ptr(new window_manager::BasicFocusRules(root)));
    window_manager_root_->accelerator_manager()->Register(
        ui::Accelerator(ui::VKEY_BROWSER_BACK, 0),
        ui::AcceleratorManager::kNormalPriority, this);
  }
  void OnViewManagerDisconnected(ViewManager* view_manager) override {
    DCHECK_EQ(view_manager_, view_manager);
    view_manager_ = NULL;
    base::MessageLoop::current()->Quit();
  }

  // Overridden from WindowManagerDelegate:
  void Embed(const String& url,
             InterfaceRequest<ServiceProvider> services,
             ServiceProviderPtr exposed_services) override {
    const Id kInvalidSourceViewId = 0;
    OnLaunch(kInvalidSourceViewId, TARGET_DEFAULT, url);
  }

  // Overridden from ui::EventHandler:
  void OnEvent(ui::Event* event) override {
    View* view =
        static_cast<window_manager::ViewTarget*>(event->target())->view();
    if (event->type() == ui::ET_MOUSE_PRESSED)
      view->SetFocus();
  }

  // Overriden from ui::AcceleratorTarget:
  bool AcceleratorPressed(const ui::Accelerator& accelerator,
                          mojo::View* view) override {
    if (accelerator.key_code() != ui::VKEY_BROWSER_BACK)
      return false;

    WindowVector::iterator iter = GetWindowByViewId(view->id());
    DCHECK(iter != windows_.end());
    Window* window = *iter;
    window->navigator_host()->RequestNavigateHistory(-1);
    return true;
  }

  // Overriden from ui::AcceleratorTarget:
  bool CanHandleAccelerators() const override { return true; }

  void OnLaunch(uint32 source_view_id,
                Target requested_target,
                const mojo::String& url) {
    Target target = navigation_target_;
    if (target == TARGET_DEFAULT) {
      if (requested_target != TARGET_DEFAULT) {
        target = requested_target;
      } else {
        // TODO(aa): Should be TARGET_NEW_NODE if source origin and dest origin
        // are different?
        target = TARGET_SOURCE_NODE;
      }
    }

    Window* dest_view = NULL;
    if (target == TARGET_SOURCE_NODE) {
      WindowVector::iterator source_view = GetWindowByViewId(source_view_id);
      bool app_initiated = source_view != windows_.end();
      if (app_initiated)
        dest_view = *source_view;
      else if (!windows_.empty())
        dest_view = windows_.back();
    }

    if (!dest_view) {
      dest_view = CreateWindow();
      windows_.push_back(dest_view);
    }

    dest_view->Embed(url);
  }

  // TODO(beng): proper layout manager!!
  Id CreateLauncherUI() {
    View* view = view_manager_->GetViewById(content_view_id_);
    Rect bounds = view->bounds();
    bounds.x += kBorderInset;
    bounds.y += kBorderInset;
    bounds.width -= 2 * kBorderInset;
    bounds.height = kTextfieldHeight;
    launcher_ui_ = CreateWindow(bounds);
    launcher_ui_->Embed("mojo:browser");
    return launcher_ui_->view()->id();
  }

  Window* CreateWindow() {
    View* view = view_manager_->GetViewById(content_view_id_);
    Rect bounds;
    bounds.x = kBorderInset;
    bounds.y = 2 * kBorderInset + kTextfieldHeight;
    bounds.width = view->bounds().width - 3 * kBorderInset - kControlPanelWidth;
    bounds.height =
        view->bounds().height - (3 * kBorderInset + kTextfieldHeight);
    if (!windows_.empty()) {
      bounds.x = windows_.back()->view()->bounds().x + 35;
      bounds.y = windows_.back()->view()->bounds().y + 35;
    }
    return CreateWindow(bounds);
  }

  Window* CreateWindow(const Rect& bounds) {
    View* content = view_manager_->GetViewById(content_view_id_);
    View* view = view_manager_->CreateView();
    content->AddChild(view);
    view->SetBounds(bounds);
    view->SetVisible(true);
    view->SetFocus();
    return new Window(this, view);
  }

  Id CreateControlPanel(View* root) {
    View* view = view_manager_->CreateView();
    root->AddChild(view);

    Rect bounds;
    bounds.x = root->bounds().width - kControlPanelWidth - kBorderInset;
    bounds.y = kBorderInset * 2 + kTextfieldHeight;
    bounds.width = kControlPanelWidth;
    bounds.height = root->bounds().height - kBorderInset * 3 - kTextfieldHeight;
    view->SetBounds(bounds);
    view->SetVisible(true);

    ServiceProviderPtr exposed_services;
    control_panel_exposed_services_impl_.Bind(GetProxy(&exposed_services));
    control_panel_exposed_services_impl_.AddService<::examples::IWindowManager>(
        this);

    GURL frame_url = url_.Resolve("/examples/window_manager/debug_panel.sky");
    view->Embed(frame_url.spec(), nullptr, exposed_services.Pass());

    return view->id();
  }

  WindowVector::iterator GetWindowByViewId(Id view_id) {
    for (std::vector<Window*>::iterator iter = windows_.begin();
         iter != windows_.end(); ++iter) {
      if ((*iter)->view()->id() == view_id) {
        return iter;
      }
    }
    return windows_.end();
  }

  Shell* shell_;

  Window* launcher_ui_;
  WindowVector windows_;
  ViewManager* view_manager_;
  scoped_ptr<RootLayoutManager> root_layout_manager_;
  ServiceProviderImpl control_panel_exposed_services_impl_;

  window_manager::WindowManagerRoot* window_manager_root_;

  // Id of the view most content is added to.
  Id content_view_id_;

  // Id of the debug panel.
  Id control_panel_id_;

  GURL url_;
  Target navigation_target_;

  ApplicationImpl* app_;

  mojo::Binding<examples::DebugPanelHost> binding_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerController);
};

class WindowManager : public ApplicationDelegate,
                      public window_manager::WindowManagerControllerFactory {
 public:
  WindowManager()
      : window_manager_app_(new window_manager::WindowManagerApp(this)) {}

  scoped_ptr<window_manager::WindowManagerController>
  CreateWindowManagerController(
      ApplicationConnection* connection,
      window_manager::WindowManagerRoot* wm_root) override {
    return scoped_ptr<WindowManagerController>(
        new WindowManagerController(shell_, app_, connection, wm_root));
  }

 private:
  // Overridden from ApplicationDelegate:
  void Initialize(ApplicationImpl* app) override {
    window_manager_app_.reset(new window_manager::WindowManagerApp(this));
    shell_ = app->shell();
    app_ = app;
    // FIXME: Mojo applications don't know their URLs yet:
    // https://docs.google.com/a/chromium.org/document/d/1AQ2y6ekzvbdaMF5WrUQmneyXJnke-MnYYL4Gz1AKDos
    url_ = GURL(app->args()[1]);
    window_manager_app_->Initialize(app);
  }

  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    window_manager_app_->ConfigureIncomingConnection(connection);
    return true;
  }

  ApplicationImpl* app_;
  Shell* shell_;
  GURL url_;

  scoped_ptr<window_manager::WindowManagerApp> window_manager_app_;
  DISALLOW_COPY_AND_ASSIGN(WindowManager);
};

void WindowManagerConnection::CloseWindow(Id view_id) {
  window_manager_->CloseWindow(view_id);
}

void NavigatorHostImpl::DidNavigateLocally(const mojo::String& url) {
  window_manager_->DidNavigateLocally(view_id_, url);
  RecordNavigation(url);
}

void NavigatorHostImpl::RequestNavigate(Target target, URLRequestPtr request) {
  window_manager_->RequestNavigate(view_id_, target, request->url);
}

void NavigatorHostImpl::RequestNavigateHistory(int32_t delta) {
  if (history_.empty())
    return;
  current_index_ =
      std::max(0, std::min(current_index_ + delta,
                           static_cast<int32_t>(history_.size()) - 1));
  window_manager_->RequestNavigate(view_id_, TARGET_SOURCE_NODE,
                                   history_[current_index_]);
}

void NavigatorHostImpl::RecordNavigation(const std::string& url) {
  if (current_index_ >= 0 && history_[current_index_] == url) {
    // This is a navigation to the current entry, ignore.
    return;
  }
  history_.erase(history_.begin() + (current_index_ + 1), history_.end());
  history_.push_back(url);
  ++current_index_;
}

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::WindowManager);
  return runner.Run(application_request);
}
