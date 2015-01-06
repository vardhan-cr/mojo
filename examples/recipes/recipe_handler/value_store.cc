// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/recipes/recipe_handler/value_store.h"

#include "base/containers/scoped_ptr_hash_map.h"
#include "examples/recipes/recipe_handler/value_store_observer.h"

namespace recipes {
namespace recipe_handler {

ValueStore::ValueStore() {
}

ValueStore::ValueStore(const Map& map) : values_(map) {
}

ValueStore::~ValueStore() {
}

void ValueStore::UpdateValues(const UpdateMap& new_values) {
  using ScopedUpdateMap = base::ScopedPtrHashMap<std::string, ValueStoreUpdate>;
  using ValueType = std::vector<uint8_t>;
  ScopedUpdateMap update_map;
  for (const auto& new_value : new_values) {
    if (!new_value.second && values_.count(new_value.first) != 0) {
      scoped_ptr<ValueStoreUpdate> update(new ValueStoreUpdate);
      update->old_value.reset(new ValueType(values_[new_value.first]));
      update_map.set(new_value.first, update.Pass());
      values_.erase(new_value.first);
    } else if (new_value.second &&
               (values_.count(new_value.first) == 0 ||
                values_[new_value.first] != *(new_value.second))) {
      scoped_ptr<ValueStoreUpdate> update(new ValueStoreUpdate);
      if (values_.count(new_value.first))
        update->old_value.reset(new ValueType(values_[new_value.first]));
      update_map.set(new_value.first, update.Pass());
      values_[new_value.first] = *(new_value.second);
    }
  }

  if (update_map.empty())
    return;

  FOR_EACH_OBSERVER(ValueStoreObserver, observers_,
                    OnValuesChanged(update_map));
}

void ValueStore::AddObserver(ValueStoreObserver* observer) {
  observers_.AddObserver(observer);
}

void ValueStore::RemoveObserver(ValueStoreObserver* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace recipe_handler
}  // namespace recipes
