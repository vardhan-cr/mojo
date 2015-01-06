// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_OBSERVER_H_
#define EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_OBSERVER_H_

#include <string>
#include <vector>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/memory/scoped_ptr.h"

namespace recipes {
namespace recipe_handler {

struct ValueStoreUpdate {
  ValueStoreUpdate();
  ~ValueStoreUpdate();

  scoped_ptr<std::vector<uint8_t>> old_value;
  scoped_ptr<std::vector<uint8_t>> new_value;
};

class ValueStoreObserver {
 public:
  virtual void OnValuesChanged(
      const base::ScopedPtrHashMap<std::string, ValueStoreUpdate>& updates) = 0;

 protected:
  virtual ~ValueStoreObserver() {}
};

}  // recipe_handler
}  // recipes

#endif  //  EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_OBSERVER_H_
