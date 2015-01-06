// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_H_
#define EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_H_

#include <map>
#include <string>
#include <vector>

#include "base/containers/scoped_ptr_hash_map.h"
#include "base/observer_list.h"

namespace recipes {
namespace recipe_handler {

class ValueStoreObserver;

// ValueStore provides a mapping from strings to a vector<uint8_t>. Use
// UpdateValues() to update the contents of the map.
class ValueStore {
 public:
  using Map = std::map<std::string, std::vector<uint8_t>>;
  using UpdateMap = base::ScopedPtrHashMap<std::string, std::vector<uint8_t>>;

  ValueStore();
  explicit ValueStore(const Map& map);
  ~ValueStore();

  // Updates this value store with that of |new_values|. Any keys in
  // |new_values| with a null value are removed from this value store,
  // otherwise the value in this value store is updated from that of the value
  // in |new_values|.
  //
  // Notifies observers if anything changed.
  void UpdateValues(const UpdateMap& new_values);

  const Map& values() const { return values_; }

  void AddObserver(ValueStoreObserver* observer);
  void RemoveObserver(ValueStoreObserver* observer);

 private:
  ObserverList<ValueStoreObserver> observers_;

  Map values_;

  DISALLOW_COPY_AND_ASSIGN(ValueStore);
};

}  // recipe_handler
}  // recipes

#endif  //  EXAMPLES_RECIPES_RECIPE_HANDLER_VALUE_STORE_H_
