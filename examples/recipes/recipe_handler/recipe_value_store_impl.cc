// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/recipes/recipe_handler/recipe_value_store_impl.h"

#include "examples/recipes/recipe_handler/value_store.h"

using ValueType = std::vector<uint8_t>;

namespace recipes {
namespace recipe_handler {

RecipeValueStoreImpl::RecipeValueStoreImpl(ValueStore* value_store)
    : value_store_(value_store) {
  value_store_->AddObserver(this);
}

RecipeValueStoreImpl::~RecipeValueStoreImpl() {
  value_store_->RemoveObserver(this);
}

void RecipeValueStoreImpl::UpdateValues(RecipeUpdateMap values) {
  if (!observer_.get())
    return;

  ValueStore::UpdateMap update_map;
  for (RecipeUpdateMap::ConstMapIterator i = values.begin(); i != values.end();
       ++i) {
    if (i.GetKey().is_null())
      update_map.set(i.GetKey(), nullptr);
    else
      update_map.set(
          i.GetKey(),
          scoped_ptr<ValueType>(new ValueType(i.GetValue().To<ValueType>())));
  }
  value_store_->UpdateValues(update_map);
}

void RecipeValueStoreImpl::SetObserver(RecipeValueStoreObserverPtr observer) {
  observer_ = observer.Pass();
  mojo::Map<mojo::String, RecipeChangeValuePtr> initial_value_map;

  for (const auto& value : value_store_->values()) {
    RecipeChangeValuePtr recipe_value(RecipeChangeValue::New());
    recipe_value->new_value = mojo::Array<uint8_t>::From(value.second);
    initial_value_map[value.first] = recipe_value.Pass();
  }
  observer_->OnValuesChanged(initial_value_map.Pass());
}

void RecipeValueStoreImpl::OnValuesChanged(const ValueStoreUpdateMap& updates) {
  if (!observer_.get())
    return;

  mojo::Map<mojo::String, RecipeChangeValuePtr> update_map;

  for (const auto& update : updates) {
    RecipeChangeValuePtr recipe_value(RecipeChangeValue::New());
    if (update.second->old_value.get())
      recipe_value->old_value =
          mojo::Array<uint8_t>::From(*update.second->old_value);
    if (update.second->new_value.get())
      recipe_value->new_value =
          mojo::Array<uint8_t>::From(*update.second->new_value);
    update_map[update.first] = recipe_value.Pass();
  }
  observer_->OnValuesChanged(update_map.Pass());
}

}  // namespace recipe_handler
}  // namespace recipes
