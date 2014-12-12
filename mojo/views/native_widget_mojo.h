// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_NATIVE_WIDGET_MOJO_H_
#define UI_VIEWS_WIDGET_NATIVE_WIDGET_MOJO_H_

#include "base/memory/weak_ptr.h"
#include "mojo/services/view_manager/public/cpp/view_observer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/compositor/layer_delegate.h"
#include "ui/compositor/layer_owner.h"
#include "ui/views/widget/native_widget_private.h"

namespace views {
class DropHelper;
class InputMethod;
class TooltipManager;
class NonClientFrameView;
class Widget;
namespace internal {
class InputMethodDelegate;
class NativeWidgetDelegate;
}
}

namespace ui {
class InputMethod;
}

namespace mojo {
class Shell;
class SurfaceContextFactory;
class View;

// A NativeWidget which interacts with a mojo::View.
class NativeWidgetMojo
    : public views::internal::NativeWidgetPrivate,
      public ui::EventHandler,
      public ViewObserver,
      public ui::LayerOwner,
      public ui::LayerDelegate {
 public:
  NativeWidgetMojo(views::internal::NativeWidgetDelegate* delegate,
                   mojo::Shell* shell,
                   mojo::View* view);

  // Overridden from internal::NativeWidgetPrivate:
  virtual void InitNativeWidget(
      const views::Widget::InitParams& params) override;
  virtual views::NonClientFrameView* CreateNonClientFrameView() override;
  virtual bool ShouldUseNativeFrame() const override;
  virtual bool ShouldWindowContentsBeTransparent() const override;
  virtual void FrameTypeChanged() override;
  virtual views::Widget* GetWidget() override;
  virtual const views::Widget* GetWidget() const override;
  virtual gfx::NativeView GetNativeView() const override;
  virtual gfx::NativeWindow GetNativeWindow() const override;
  virtual views::Widget* GetTopLevelWidget() override;
  virtual const ui::Compositor* GetCompositor() const override;
  virtual ui::Compositor* GetCompositor() override;
  virtual ui::Layer* GetLayer() override;
  virtual void ReorderNativeViews() override;
  virtual void ViewRemoved(views::View* view) override;
  virtual void SetNativeWindowProperty(const char* name, void* value) override;
  virtual void* GetNativeWindowProperty(const char* name) const override;
  virtual views::TooltipManager* GetTooltipManager() const override;
  virtual void SetCapture() override;
  virtual void ReleaseCapture() override;
  virtual bool HasCapture() const override;
  virtual views::InputMethod* CreateInputMethod() override;
  virtual views::internal::InputMethodDelegate* GetInputMethodDelegate()
      override;
  virtual ui::InputMethod* GetHostInputMethod() override;
  virtual void CenterWindow(const gfx::Size& size) override;
  virtual void GetWindowPlacement(
      gfx::Rect* bounds,
      ui::WindowShowState* maximized) const override;
  virtual bool SetWindowTitle(const base::string16& title) override;
  virtual void SetWindowIcons(const gfx::ImageSkia& window_icon,
                              const gfx::ImageSkia& app_icon) override;
  virtual void InitModalType(ui::ModalType modal_type) override;
  virtual gfx::Rect GetWindowBoundsInScreen() const override;
  virtual gfx::Rect GetClientAreaBoundsInScreen() const override;
  virtual gfx::Rect GetRestoredBounds() const override;
  virtual void SetBounds(const gfx::Rect& bounds) override;
  virtual void SetSize(const gfx::Size& size) override;
  virtual void StackAbove(gfx::NativeView native_view) override;
  virtual void StackAtTop() override;
  virtual void StackBelow(gfx::NativeView native_view) override;
  virtual void SetShape(gfx::NativeRegion shape) override;
  virtual void Close() override;
  virtual void CloseNow() override;
  virtual void Show() override;
  virtual void Hide() override;
  virtual void ShowMaximizedWithBounds(
      const gfx::Rect& restored_bounds) override;
  virtual void ShowWithWindowState(ui::WindowShowState state) override;
  virtual bool IsVisible() const override;
  virtual void Activate() override;
  virtual void Deactivate() override;
  virtual bool IsActive() const override;
  virtual void SetAlwaysOnTop(bool always_on_top) override;
  virtual bool IsAlwaysOnTop() const override;
  virtual void SetVisibleOnAllWorkspaces(bool always_visible) override;
  virtual void Maximize() override;
  virtual void Minimize() override;
  virtual bool IsMaximized() const override;
  virtual bool IsMinimized() const override;
  virtual void Restore() override;
  virtual void SetFullscreen(bool fullscreen) override;
  virtual bool IsFullscreen() const override;
  virtual void SetOpacity(unsigned char opacity) override;
  virtual void SetUseDragFrame(bool use_drag_frame) override;
  virtual void FlashFrame(bool flash_frame) override;
  virtual void RunShellDrag(views::View* view,
                            const ui::OSExchangeData& data,
                            const gfx::Point& location,
                            int operation,
                            ui::DragDropTypes::DragEventSource source) override;
  virtual void SchedulePaintInRect(const gfx::Rect& rect) override;
  virtual void SetCursor(gfx::NativeCursor cursor) override;
  virtual bool IsMouseEventsEnabled() const override;
  virtual void ClearNativeFocus() override;
  virtual gfx::Rect GetWorkAreaBoundsInScreen() const override;
  virtual views::Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      views::Widget::MoveLoopSource source,
      views::Widget::MoveLoopEscapeBehavior escape_behavior) override;
  virtual void EndMoveLoop() override;
  virtual void SetVisibilityChangedAnimationsEnabled(bool value) override;
  virtual ui::NativeTheme* GetNativeTheme() const override;
  virtual void OnRootViewLayout() override;
  virtual bool IsTranslucentWindowOpacitySupported() const override;
  virtual void OnSizeConstraintsChanged() override;
  virtual void RepostNativeEvent(gfx::NativeEvent native_event) override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // TODO(erg): Plumb activation events from the window manager to here. We
  // should receive the equivalent of OnWindowActivated() and OnWindowFocused()
  // here.

  // TODO(erg): Handle drag/drop. Maybe an equivalent of
  // aura::client::DragDropDelegate? Or a different design?

  // Overridden from ViewObserver:
  void OnViewDestroying(View* view) override;
  void OnViewDestroyed(View* view) override;
  void OnViewBoundsChanged(View* view,
                           const Rect& old_bounds,
                           const Rect& new_bounds) override;
  void OnViewFocusChanged(View* gained_focus, View* lost_focus) override;
  void OnViewInputEvent(View* view, const EventPtr& event) override;

  // Overridden from LayerDelegate:
  void OnPaintLayer(gfx::Canvas* canvas) override;
  void OnDelegatedFrameDamage(const gfx::Rect& damage_rect_in_dip) override;
  void OnDeviceScaleFactorChanged(float device_scale_factor) override;
  base::Closure PrepareForLayerBoundsChange() override;

 protected:
  virtual ~NativeWidgetMojo();

  views::internal::NativeWidgetDelegate* delegate() { return delegate_; }

 private:
  class ActiveWindowObserver;

  void SetInitialFocus(ui::WindowShowState show_state);

  views::internal::NativeWidgetDelegate* delegate_;

  // See class documentation for Widget in widget.h for a note about ownership.
  views::Widget::InitParams::Ownership ownership_;

  mojo::View* view_;

  // Are we in the destructor?
  bool destroying_;

  scoped_ptr<SurfaceContextFactory> context_factory_;
  scoped_ptr<ui::Compositor> compositor_;

  // The following factory is used for calls to close the NativeWidgetMojo
  // instance.
  base::WeakPtrFactory<NativeWidgetMojo> close_widget_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetMojo);
};

}  // namespace mojo

#endif  // UI_VIEWS_WIDGET_NATIVE_WIDGET_MOJO_H_
