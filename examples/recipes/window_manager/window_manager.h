// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_WINDOW_MANAGER_WINDOW_MANAGER_H_
#define EXAMPLES_RECIPES_WINDOW_MANAGER_WINDOW_MANAGER_H_

#include <set>
#include <vector>

#include "mojo/services/view_manager/public/cpp/view_observer.h"

namespace mojo {
class View;
class ViewManager;
}

namespace recipes {
namespace window_manager {

// Manages the set of immersive and transient views as used by recipes. The set
// of immersive views is maintained as a list (|immersive_views_|). At most one
// immersive view is shown at a time. The active immersive view changes by
// making an immersive view visible (can be initiated by a client). The active
// immersive view implicitly changes if the active immersive view is
// hidden. When this happens the previous immersive view is the list is shown.
//
// In addition to an immersive view there can be a transient view that is placed
// on top of the active immersive view.
//
// Immersive and transient views are created by way of the Create() function.
// This function is called in response to the Embed() function of the
// WindowManager interface. The type of window (immersive or transient) is set
// by way of |kWindowTypeKey|.
class WindowManager : public mojo::ViewObserver {
 public:
  // Create a new WindowManager. |root| is the root view all immersive views are
  // added to.
  explicit WindowManager(mojo::View* root);
  ~WindowManager() override;

  // Creates an immersive of transient view.
  mojo::View* Create();

  mojo::View* active_immersive_view() { return active_immersive_view_; }
  mojo::View* active_transient_view() { return active_transient_view_; }

 private:
  using ImmersiveViews = std::vector<mojo::View*>;

  // Updates the bounds of |view|. This is invoked during construction and when
  // the window type changes.
  void UpdateBounds(mojo::View* view);

  void SetActiveTransient(mojo::View* view);
  void SetActiveImmersive(mojo::View* view);

  void HideImmersive(mojo::View* view);
  void HideActiveTransient();

  // Shows the previously immersive view.
  void ShowPreviousImmersive();

  // Invoked when a view should no longer be active, such as when a client hides
  // a view or it's deleted.
  void ProcessViewShouldNoLongerBeActive(mojo::View* view);

  // mojo::ViewObserver:
  void OnViewDestroying(mojo::View* view) override;
  void OnViewVisibilityChanged(mojo::View* view) override;
  void OnViewSharedPropertyChanged(
      mojo::View* view,
      const std::string& name,
      const std::vector<uint8_t>* old_data,
      const std::vector<uint8_t>* new_data) override;
  void OnViewEmbeddedAppDisconnected(mojo::View* view) override;

  // Immersive and transient views are added as children of this.
  mojo::View* root_;

  // Ordered set of immersive views.
  ImmersiveViews immersive_views_;

  mojo::View* active_immersive_view_;
  mojo::View* active_transient_view_;

  // This is the set of views returned by Create(). These will either be
  // immersive or transient.
  std::set<mojo::View*> views_;

  DISALLOW_COPY_AND_ASSIGN(WindowManager);
};

}  // window_manager
}  // recipes

#endif  // EXAMPLES_RECIPES_WINDOW_MANAGER_WINDOW_MANAGER_H_
