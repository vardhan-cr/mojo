// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_VALUE_STORE_IMPL_H_
#define EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_VALUE_STORE_IMPL_H_

#include "examples/recipes/recipe_handler/recipe_value_store.mojom.h"
#include "examples/recipes/recipe_handler/value_store_observer.h"

namespace recipes {
namespace recipe_handler {

class ValueStore;

class RecipeValueStoreImpl : public ValueStoreObserver,
                             public RecipeValueStore {
 public:
  explicit RecipeValueStoreImpl(ValueStore* value_store);
  ~RecipeValueStoreImpl() override;

 private:
  using RecipeUpdateMap = mojo::Map<mojo::String, mojo::Array<uint8_t>>;
  using ValueStoreUpdateMap =
      base::ScopedPtrHashMap<std::string, ValueStoreUpdate>;

  // RecipeValueStore:
  void UpdateValues(RecipeUpdateMap values) override;
  void SetObserver(RecipeValueStoreObserverPtr observer) override;

  // ValueStoreObserver:
  void OnValuesChanged(const ValueStoreUpdateMap& updates) override;

  ValueStore* value_store_;

  RecipeValueStoreObserverPtr observer_;

  DISALLOW_COPY_AND_ASSIGN(RecipeValueStoreImpl);
};

}  // recipe_handler
}  // recipes

#endif  //  EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_VALUE_STORE_IMPL_H_
