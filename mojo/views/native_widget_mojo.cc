// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/views/native_widget_mojo.h"

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "mojo/aura/surface_context_factory.h"
#include "mojo/converters/geometry/geometry_type_converters.h"
#include "mojo/converters/input_events/input_events_type_converters.h"
#include "ui/compositor/compositor.h"
#include "ui/events/event.h"
#include "ui/native_theme/native_theme_aura.h"
#include "ui/views/ime/input_method.h"
#include "ui/views/views_delegate.h"
#include "ui/views/widget/native_widget_delegate.h"
#include "ui/views/widget/widget_delegate.h"

namespace gfx {
class Canvas;
}

namespace mojo {

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, public:

NativeWidgetMojo::NativeWidgetMojo(
    views::internal::NativeWidgetDelegate* delegate,
    mojo::Shell* shell,
    mojo::View* view)
    : delegate_(delegate),
      ownership_(views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET),
      view_(view),
      destroying_(false),
      close_widget_factory_(this) {
  context_factory_.reset(new SurfaceContextFactory(shell, view));
  compositor_.reset(new ui::Compositor(gfx::kNullAcceleratedWidget,
                                       context_factory_.get(),
                                       base::ThreadTaskRunnerHandle::Get()));

  gfx::Size size = view->bounds().To<gfx::Rect>().size();
  compositor_->SetScaleAndSize(1.0f, size);
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, internal::NativeWidgetPrivate implementation:

void NativeWidgetMojo::InitNativeWidget(
    const views::Widget::InitParams& params) {
  ownership_ = params.ownership;

  SetLayer(new ui::Layer(ui::LAYER_TEXTURED));
  layer()->SetVisible(true);
  layer()->set_delegate(this);
  compositor_->SetRootLayer(layer());

  // TODO(erg): The aura version sets shadows types on their window here.
  if (params.type == views::Widget::InitParams::TYPE_CONTROL)
    view_->SetVisible(true);

  delegate_->OnNativeWidgetCreated(false);

  gfx::Rect window_bounds = params.bounds;
  // TODO(erg): Handle params.child once we have child widgets, along with
  // params.parent.

  // Set properties before adding to the parent so that its layout manager sees
  // the correct values.
  OnSizeConstraintsChanged();

  // TODO(erg): Handle parent relations here.

  // Start observing property changes.
  view_->AddObserver(this);

  // TODO(erg): Do additional things related to the restore bounds and
  // maximization. See NWA.
  SetBounds(window_bounds);
}

views::NonClientFrameView* NativeWidgetMojo::CreateNonClientFrameView() {
  return nullptr;
}

bool NativeWidgetMojo::ShouldUseNativeFrame() const {
  // TODO(erg): We can probably remove this method; it's only used in windows
  // for Aero frames.
  return false;
}

bool NativeWidgetMojo::ShouldWindowContentsBeTransparent() const {
  return false;
}

void NativeWidgetMojo::FrameTypeChanged() {
  // This is called when the Theme has changed; forward the event to the root
  // widget.
  GetWidget()->ThemeChanged();
  GetWidget()->GetRootView()->SchedulePaint();
}

views::Widget* NativeWidgetMojo::GetWidget() {
  return delegate_->AsWidget();
}

const views::Widget* NativeWidgetMojo::GetWidget() const {
  return delegate_->AsWidget();
}

gfx::NativeView NativeWidgetMojo::GetNativeView() const {
  // TODO(erg): Actually implementing this will require moving off aura
  // entirely because of the NativeView typedefs. (Those will need to be
  // changed to mojo::View being the native view/window.)
  return nullptr;
}

gfx::NativeWindow NativeWidgetMojo::GetNativeWindow() const {
  return nullptr;
}

views::Widget* NativeWidgetMojo::GetTopLevelWidget() {
  NativeWidgetPrivate* native_widget = GetTopLevelNativeWidget(GetNativeView());
  return native_widget ? native_widget->GetWidget() : nullptr;
}

const ui::Compositor* NativeWidgetMojo::GetCompositor() const {
  return compositor_.get();
}

ui::Compositor* NativeWidgetMojo::GetCompositor() {
  return compositor_.get();
}

ui::Layer* NativeWidgetMojo::GetLayer() {
  return layer();
}

void NativeWidgetMojo::ReorderNativeViews() {
  // TODO(erg): Punting on this until we actually have something that uses
  // ui::Layer or NativeViewHost.
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::ViewRemoved(views::View* view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::SetNativeWindowProperty(const char* name, void* value) {
  // TODO(erg): "Native Window" properties do not actually set things on the
  // system's native window; in mojo::View parlance, this is actually a local
  // property, not a shared one.
  NOTIMPLEMENTED();
}

void* NativeWidgetMojo::GetNativeWindowProperty(const char* name) const {
  NOTIMPLEMENTED();
  return nullptr;
}

views::TooltipManager* NativeWidgetMojo::GetTooltipManager() const {
  NOTIMPLEMENTED();
  return nullptr;
}

void NativeWidgetMojo::SetCapture() {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::ReleaseCapture() {
  NOTIMPLEMENTED();
}

bool NativeWidgetMojo::HasCapture() const {
  NOTIMPLEMENTED();
  return false;
}

views::InputMethod* NativeWidgetMojo::CreateInputMethod() {
  // TODO(erg): Implement this; having a NOTIMPLEMENTED() spews too much.
  return nullptr;
}

views::internal::InputMethodDelegate*
NativeWidgetMojo::GetInputMethodDelegate() {
  NOTIMPLEMENTED();
  return nullptr;
}

ui::InputMethod* NativeWidgetMojo::GetHostInputMethod() {
  NOTIMPLEMENTED();
  return nullptr;
}

void NativeWidgetMojo::CenterWindow(const gfx::Size& size) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::GetWindowPlacement(
    gfx::Rect* bounds,
    ui::WindowShowState* show_state) const {
  // The interface specifies returning restored bounds, not current bounds.
  *bounds = GetRestoredBounds();
  // TODO(erg): Redo how we encode |show_state|.
}

bool NativeWidgetMojo::SetWindowTitle(const base::string16& title) {
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMojo::SetWindowIcons(const gfx::ImageSkia& window_icon,
                                      const gfx::ImageSkia& app_icon) {
  // TODO(erg): If we add an interface for this, we should change the method
  // type to pass a representation of different icon sizes. The current
  // signature is a legacy of how Windows passes icons around.
}

void NativeWidgetMojo::InitModalType(ui::ModalType modal_type) {
  NOTIMPLEMENTED();
}

gfx::Rect NativeWidgetMojo::GetWindowBoundsInScreen() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

gfx::Rect NativeWidgetMojo::GetClientAreaBoundsInScreen() const {
  // View-to-screen coordinate system transformations depend on this returning
  // the full window bounds, for example View::ConvertPointToScreen().
  NOTIMPLEMENTED();
  return gfx::Rect();
}

gfx::Rect NativeWidgetMojo::GetRestoredBounds() const {
  NOTIMPLEMENTED();
  return gfx::Rect();
}

void NativeWidgetMojo::SetBounds(const gfx::Rect& bounds) {
  if (!view_)
    return;

  view_->SetBounds(*mojo::Rect::From(bounds));

  gfx::Size size = bounds.size();
  compositor_->SetScaleAndSize(1.0f, size);
  layer()->SetBounds(gfx::Rect(size));
  delegate_->OnNativeWidgetSizeChanged(size);
}

void NativeWidgetMojo::SetSize(const gfx::Size& size) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::StackAbove(gfx::NativeView native_view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::StackAtTop() {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::StackBelow(gfx::NativeView native_view) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::SetShape(gfx::NativeRegion region) {
  // TODO(erg): This is incomplete here. We also need to set the shape on the
  // mojo::View.
  NOTIMPLEMENTED();
  if (view_)
    layer()->SetAlphaShape(make_scoped_ptr(region));
  else
    delete region;
}

void NativeWidgetMojo::Close() {
  // |window_| may already be deleted by parent window. This can happen
  // when this widget is child widget or has transient parent
  // and ownership is WIDGET_OWNS_NATIVE_WIDGET.
  DCHECK(view_ ||
         ownership_ == views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET);
  if (view_) {
    Hide();
    // TODO(erg): Undo window modality here.
  }

  if (!close_widget_factory_.HasWeakPtrs()) {
    base::MessageLoop::current()->PostTask(
        FROM_HERE, base::Bind(&NativeWidgetMojo::CloseNow,
                              close_widget_factory_.GetWeakPtr()));
  }
}

void NativeWidgetMojo::CloseNow() {
  view_->Destroy();
}

void NativeWidgetMojo::Show() {
  ShowWithWindowState(ui::SHOW_STATE_NORMAL);
}

void NativeWidgetMojo::Hide() {
  if (view_)
    view_->SetVisible(false);
}

void NativeWidgetMojo::ShowMaximizedWithBounds(
    const gfx::Rect& restored_bounds) {
  // TODO(erg): Save the restored bounds.
  ShowWithWindowState(ui::SHOW_STATE_MAXIMIZED);
}

void NativeWidgetMojo::ShowWithWindowState(ui::WindowShowState state) {
  if (!view_)
    return;

  // TODO(erg): Set |show_state|, if it still makes sense.
  view_->SetVisible(true);
  if (delegate_->CanActivate()) {
    if (state != ui::SHOW_STATE_INACTIVE)
      Activate();
    // SetInitialFocus() should be always be called, even for
    // SHOW_STATE_INACTIVE. If the window has to stay inactive, the method will
    // do the right thing.
    SetInitialFocus(state);
  }
}

bool NativeWidgetMojo::IsVisible() const {
  return view_ && view_->visible();
}

void NativeWidgetMojo::Activate() {
  // TODO(erg): Need to plumb activation to the window manager here.
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::Deactivate() {
  // TODO(erg): Need to plumb deactivation to the window manager here.
  NOTIMPLEMENTED();
}

bool NativeWidgetMojo::IsActive() const {
  return true;
}

void NativeWidgetMojo::SetAlwaysOnTop(bool on_top) {
  // TODO(erg): Plumb this to the window manager.
  NOTIMPLEMENTED();
}

bool NativeWidgetMojo::IsAlwaysOnTop() const {
  return false;
}

void NativeWidgetMojo::SetVisibleOnAllWorkspaces(bool always_visible) {
  // Not implemented on chromeos or for child widgets.
}

void NativeWidgetMojo::Maximize() {
  // TODO(erg): Everything about minimization/maximization should really be
  // part of the window manager; these methods should be replaced with
  // something like "RequestMaximize()".
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::Minimize() {
  NOTIMPLEMENTED();
}

bool NativeWidgetMojo::IsMaximized() const {
  return false;
}

bool NativeWidgetMojo::IsMinimized() const {
  return false;
}

void NativeWidgetMojo::Restore() {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::SetFullscreen(bool fullscreen) {
  NOTIMPLEMENTED();
}

bool NativeWidgetMojo::IsFullscreen() const {
  return false;
}

void NativeWidgetMojo::SetOpacity(unsigned char opacity) {
  // TODO(erg): Also need this on mojo::View.
  if (layer())
    layer()->SetOpacity(opacity / 255.0);
}

void NativeWidgetMojo::SetUseDragFrame(bool use_drag_frame) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::FlashFrame(bool flash) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::RunShellDrag(views::View* view,
                                    const ui::OSExchangeData& data,
                                    const gfx::Point& location,
                                    int operation,
                                    ui::DragDropTypes::DragEventSource source) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::SchedulePaintInRect(const gfx::Rect& rect) {
  layer()->SchedulePaint(rect);
}

void NativeWidgetMojo::SetCursor(gfx::NativeCursor cursor) {
  // TODO(erg): Requires a cursor client. Is so spammy that we don't put down a
  // NOTIMPLEMENTED().
}

bool NativeWidgetMojo::IsMouseEventsEnabled() const {
  // TODO(erg): Requires a cursor client.
  NOTIMPLEMENTED();
  return true;
}

void NativeWidgetMojo::ClearNativeFocus() {
  // TODO(erg): Requires an aura::client::FocusClient equivalent.
}

gfx::Rect NativeWidgetMojo::GetWorkAreaBoundsInScreen() const {
  // TODO(erg): Requires a gfx::Screen equivalent.
  return gfx::Rect();
}

views::Widget::MoveLoopResult NativeWidgetMojo::RunMoveLoop(
    const gfx::Vector2d& drag_offset,
    views::Widget::MoveLoopSource source,
    views::Widget::MoveLoopEscapeBehavior escape_behavior) {
  // TODO(erg): This is really complicated. Requires an
  // aura::client::WindowMoveClient equivalent.
  NOTIMPLEMENTED();
  return views::Widget::MOVE_LOOP_CANCELED;
}

void NativeWidgetMojo::EndMoveLoop() {
  // TODO(erg): Requires an aura::client::WindowMoveClient equivalent.
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::SetVisibilityChangedAnimationsEnabled(bool value) {
  NOTIMPLEMENTED();
}

ui::NativeTheme* NativeWidgetMojo::GetNativeTheme() const {
  // TODO(erg): NativeThemeAura does not actually use aura. When we remove
  // aura, this should be renamed to not be confusing.
  return ui::NativeThemeAura::instance();
}

void NativeWidgetMojo::OnRootViewLayout() {
}

bool NativeWidgetMojo::IsTranslucentWindowOpacitySupported() const {
  return true;
}

void NativeWidgetMojo::OnSizeConstraintsChanged() {
  // TODO(erg): In the aura version, we set Can{Maximize,Minimize,Resize}
  // properties on our backing window from the widget_delegate(). Figure out
  // what we do policy wise here.
}

void NativeWidgetMojo::RepostNativeEvent(gfx::NativeEvent native_event) {
  // TODO(erg): Remove this method from the whole views system. The one caller
  // is in WebDialogView, which I assume we'll never use in mojo.
  NOTREACHED();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, ui::EventHandler implementation:

void NativeWidgetMojo::OnKeyEvent(ui::KeyEvent* event) {
  // TODO(erg): Key event handling is a pile of hurt. Rebuilding all of this
  // means dealing with the IME. For now, punt.
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::OnMouseEvent(ui::MouseEvent* event) {
  DCHECK(view_);
  DCHECK(view_->visible());
  if (event->type() == ui::ET_MOUSEWHEEL) {
    delegate_->OnMouseEvent(event);
    if (event->handled())
      return;
  }

  // TODO(erg): In the aura version, we do something with the
  // ToopltipManagerAura here.
  delegate_->OnMouseEvent(event);
}

void NativeWidgetMojo::OnScrollEvent(ui::ScrollEvent* event) {
  delegate_->OnScrollEvent(event);
}

void NativeWidgetMojo::OnGestureEvent(ui::GestureEvent* event) {
  DCHECK(view_);
  DCHECK(view_->visible() || event->IsEndingEvent());
  delegate_->OnGestureEvent(event);
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, ViewObserver implementation:

void NativeWidgetMojo::OnViewDestroying(View* view) {
  view->RemoveObserver(this);
  delegate_->OnNativeWidgetDestroying();
}

void NativeWidgetMojo::OnViewDestroyed(View* view) {
  DCHECK_EQ(view, view_);
  view_ = nullptr;
  delegate_->OnNativeWidgetDestroyed();
  if (ownership_ == views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete this;
}

void NativeWidgetMojo::OnViewBoundsChanged(View* view,
                                           const Rect& mojo_old_bounds,
                                           const Rect& mojo_new_bounds) {
  gfx::Rect old_bounds = mojo_old_bounds.To<gfx::Rect>();
  gfx::Rect new_bounds = mojo_new_bounds.To<gfx::Rect>();

  gfx::Size size = new_bounds.size();
  compositor_->SetScaleAndSize(1.0f, size);
  layer()->SetBounds(gfx::Rect(size));

  // Assume that if the old bounds was completely empty a move happened. This
  // handles the case of a maximize animation acquiring the layer (acquiring a
  // layer results in clearing the bounds).
  if (old_bounds.origin() != new_bounds.origin() ||
      (old_bounds == gfx::Rect(0, 0, 0, 0) && !new_bounds.IsEmpty())) {
    delegate_->OnNativeWidgetMove();
  }
  if (old_bounds.size() != new_bounds.size())
    delegate_->OnNativeWidgetSizeChanged(new_bounds.size());
}

void NativeWidgetMojo::OnViewFocusChanged(View* gained_focus,
                                          View* lost_focus) {
  if (view_ == gained_focus) {
    if (GetWidget()->GetInputMethod())  // Null in tests.
      GetWidget()->GetInputMethod()->OnFocus();

    // TODO(erg): We need to set mojo::View* as the gfx::NativeView. Once that
    // is done, we can uncomment the line below.
    //
    // delegate_->OnNativeFocus(lost_focus);
  } else if (view_ == lost_focus) {
    // GetInputMethod() recreates the input method if it's previously been
    // destroyed.  If we get called during destruction, the input method will be
    // gone, and creating a new one and telling it that we lost the focus will
    // trigger a DCHECK (the new input method doesn't think that we have the
    // focus and doesn't expect a blur).  OnBlur() shouldn't be called during
    // destruction unless WIDGET_OWNS_NATIVE_WIDGET is set (which is just the
    // case in tests).
    if (!destroying_) {
      if (GetWidget()->GetInputMethod())
        GetWidget()->GetInputMethod()->OnBlur();
    } else {
      DCHECK_EQ(ownership_,
                views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET);
    }

    // TODO(erg): We need to set mojo::View* as the gfx::NativeView. Once that
    // is done, we can uncomment the line below.
    //
    // delegate_->OnNativeBlur(gained_focus);
  }
}

void NativeWidgetMojo::OnViewInputEvent(View* view, const EventPtr& event) {
  scoped_ptr<ui::Event> ui_event(event.To<scoped_ptr<ui::Event>>());
  if (ui_event)
    OnEvent(ui_event.get());
}

// TODO(erg): Add an OnViewActivationChanged() method and plumb that from the
// window manager through the view manager to here.

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, LayerDelegate:

void NativeWidgetMojo::OnPaintLayer(gfx::Canvas* canvas) {
  delegate_->OnNativeWidgetPaint(canvas);
}

void NativeWidgetMojo::OnDelegatedFrameDamage(
    const gfx::Rect& damage_rect_in_dip) {
  NOTIMPLEMENTED();
}

void NativeWidgetMojo::OnDeviceScaleFactorChanged(float device_scale_factor) {
  NOTIMPLEMENTED();
}

base::Closure NativeWidgetMojo::PrepareForLayerBoundsChange() {
  // TODO(erg): This needs a real implementation, but we don't NOTIMPLEMENTED()
  // because spam.
  return base::Closure();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, protected:

NativeWidgetMojo::~NativeWidgetMojo() {
  destroying_ = true;
  if (ownership_ == views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET)
    delete delegate_;
  else
    CloseNow();
}

////////////////////////////////////////////////////////////////////////////////
// NativeWidgetMojo, private:

void NativeWidgetMojo::SetInitialFocus(ui::WindowShowState show_state) {
  NOTIMPLEMENTED();
}

namespace internal {

////////////////////////////////////////////////////////////////////////////////
// internal::NativeWidgetPrivate, public:

// TODO(erg): Re-implement the static NativeWidgetPrivate interface here. This
// is already implemented in NativeWidgetAura, and we can't reimplement it here
// until we delete it there.

}  // namespace internal
}  // namespace mojo
