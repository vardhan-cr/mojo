// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/recipes/window_manager/window_manager.h"

#include <algorithm>

#include "examples/recipes/window_manager/constants.h"
#include "mojo/services/view_manager/public/cpp/view_manager.h"
#include "mojo/services/view_manager/public/cpp/view_property.h"

using mojo::View;

namespace recipes {
namespace window_manager {
namespace {

enum WindowType {
  WINDOW_TYPE_IMMERSIVE,
  WINDOW_TYPE_TRANSIENT,
};

// For the time being we only support changing the type before the initial show.
// We could make changing at any point work, but I don't think it's very useful
// right now, so the value is cached locally.
DEFINE_LOCAL_VIEW_PROPERTY_KEY(WindowType,
                               kWindowTypeLocalKey,
                               WINDOW_TYPE_IMMERSIVE);

}  // namespace

WindowManager::WindowManager(View* root)
    : root_(root),
      active_immersive_view_(nullptr),
      active_transient_view_(nullptr) {
}

WindowManager::~WindowManager() {
  for (View* view : views_)
    view->RemoveObserver(this);
}

View* WindowManager::Create() {
  View* view = root_->view_manager()->CreateView();
  root_->AddChild(view);
  UpdateBounds(view);
  view->AddObserver(this);
  views_.insert(view);
  return view;
}

void WindowManager::UpdateBounds(View* view) {
  mojo::Rect bounds(root_->bounds());
  bounds.x = bounds.y = 0;
  if (view->GetLocalProperty(kWindowTypeLocalKey) == WINDOW_TYPE_TRANSIENT) {
    bounds.x += 20;
    bounds.y += 20;
    bounds.width -= 40;
    bounds.height -= 40;
  }
  view->SetBounds(bounds);
}

void WindowManager::SetActiveTransient(View* view) {
  if (view == active_transient_view_)
    return;

  HideActiveTransient();
  active_transient_view_ = view;
  if (active_transient_view_->parent() != root_)
    root_->AddChild(active_transient_view_);
  active_transient_view_->MoveToFront();
  active_transient_view_->SetVisible(true);
  active_transient_view_->SetFocus();
}

void WindowManager::SetActiveImmersive(View* view) {
  if (view == active_immersive_view_)
    return;

  DCHECK(view);
  HideActiveTransient();

  {
    ImmersiveViews::iterator i =
        std::find(immersive_views_.begin(), immersive_views_.end(), view);
    if (i == immersive_views_.end())
      immersive_views_.push_back(view);
  }

  if (active_immersive_view_) {
    View* old_active = active_immersive_view_;
    active_immersive_view_ = nullptr;
    old_active->SetVisible(false);
  }

  active_immersive_view_ = view;
  if (active_immersive_view_->parent() != root_)
    root_->AddChild(active_immersive_view_);
  active_immersive_view_->MoveToFront();
  active_immersive_view_->SetVisible(true);
  active_immersive_view_->SetFocus();
}

void WindowManager::HideActiveTransient() {
  if (!active_transient_view_)
    return;
  active_transient_view_->SetVisible(false);
  active_transient_view_ = nullptr;
}

void WindowManager::ShowPreviousImmersive() {
  if (!active_immersive_view_)
    return;

  ImmersiveViews::iterator i = std::find(
      immersive_views_.begin(), immersive_views_.end(), active_immersive_view_);
  DCHECK(i != immersive_views_.end());
  const size_t index = i - immersive_views_.end();
  if (index == 0) {
    if (index + 1 == immersive_views_.size())
      HideActiveTransient();
    else
      SetActiveImmersive(immersive_views_[1]);
  } else {
    SetActiveImmersive(immersive_views_[index - 1]);
  }
}

void WindowManager::ProcessViewShouldNoLongerBeActive(mojo::View* view) {
  if (view == active_immersive_view_) {
    ShowPreviousImmersive();
  } else if (view == active_transient_view_) {
    HideActiveTransient();
  }
}

void WindowManager::OnViewDestroying(View* view) {
  DCHECK(view != active_immersive_view_);
  DCHECK(view != active_transient_view_);
  {
    ImmersiveViews::iterator i =
        std::find(immersive_views_.begin(), immersive_views_.end(), view);
    if (i != immersive_views_.end())
      immersive_views_.erase(i);
  }
  views_.erase(view);
  view->RemoveObserver(this);
}

void WindowManager::OnViewVisibilityChanged(View* view) {
  if (!view->visible()) {
    ProcessViewShouldNoLongerBeActive(view);
    return;
  }

  if (view->GetLocalProperty(kWindowTypeLocalKey) == WINDOW_TYPE_TRANSIENT) {
    SetActiveTransient(view);
    return;
  }

  // A client is requesting an immersive view to become active.
  // TODO(sky): figure out if we want to reorder |immersive_views_| here.
  SetActiveImmersive(view);
}

void WindowManager::OnViewSharedPropertyChanged(
    View* view,
    const std::string& name,
    const std::vector<uint8_t>* old_data,
    const std::vector<uint8_t>* new_data) {
  if (name == kWindowTypeKey) {
    if (view->parent() != nullptr) {
      // See comment in kWindowTypeLocalKey for details.
      NOTIMPLEMENTED();
      return;
    }
    View::SharedProperties::const_iterator i =
        view->shared_properties().find(kWindowTypeKey);
    const WindowType window_type =
        i != view->shared_properties().end() &&
                (std::string(&i->second.front(),
                             &i->second.front() + i->second.size()) ==
                 kWindowTypeValueTransient)
            ? WINDOW_TYPE_TRANSIENT
            : WINDOW_TYPE_IMMERSIVE;
    view->SetLocalProperty(kWindowTypeLocalKey, window_type);
    UpdateBounds(view);
  }
}

void WindowManager::OnViewEmbeddedAppDisconnected(View* view) {
  ProcessViewShouldNoLongerBeActive(view);
  view->Destroy();
}

}  // window_manager
}  // recipes
