// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_VIEW_MANAGER_SERVER_VIEW_OBSERVER_H_
#define SERVICES_VIEW_MANAGER_SERVER_VIEW_OBSERVER_H_

#include "mojo/services/view_manager/public/interfaces/view_manager_constants.mojom.h"

namespace gfx {
class Rect;
}

namespace mojo {
class ViewportMetrics;
}

namespace view_manager {

class ServerView;

class ServerViewObserver {
 public:
  // Invoked when a view is about to be destroyed; before any of the children
  // have been removed and before the view has been removed from its parent.
  virtual void OnWillDestroyView(const ServerView* view) {}

  // Invoked at the end of the View's destructor (after it has been removed from
  // the hierarchy).
  virtual void OnViewDestroyed(const ServerView* view) {}

  virtual void OnWillChangeViewHierarchy(const ServerView* view,
                                         const ServerView* new_parent,
                                         const ServerView* old_parent) {}

  virtual void OnViewHierarchyChanged(const ServerView* view,
                                      const ServerView* new_parent,
                                      const ServerView* old_parent) {}

  virtual void OnViewBoundsChanged(const ServerView* view,
                                   const gfx::Rect& old_bounds,
                                   const gfx::Rect& new_bounds) {}

  virtual void OnViewReordered(const ServerView* view,
                               const ServerView* relative,
                               mojo::OrderDirection direction) {}

  virtual void OnWillChangeViewVisibility(const ServerView* view) {}

  virtual void OnViewSharedPropertyChanged(
      const ServerView* view,
      const std::string& name,
      const std::vector<uint8_t>* new_data) {}

 protected:
  virtual ~ServerViewObserver() {}
};

}  // namespace view_manager

#endif  // SERVICES_VIEW_MANAGER_SERVER_VIEW_OBSERVER_H_
