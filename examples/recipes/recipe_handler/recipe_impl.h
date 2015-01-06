// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_IMPL_H_
#define EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_IMPL_H_

#include <vector>

#include "base/memory/scoped_ptr.h"
#include "base/memory/scoped_vector.h"
#include "examples/recipes/recipe_handler/recipe.mojom.h"
#include "examples/recipes/recipe_handler/recipe_value_store.mojom.h"
#include "examples/recipes/recipe_handler/value_store.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "url/gurl.h"

namespace base {
class Value;
}

namespace recipes {
namespace recipe_handler {

class RecipeValueStoreImpl;

// In addition to the Recipe interface RecipeImpl maintains the connections to
// the ingredients that make up the recipe. IngredientConnection is used for
// each connection and exposes the RecipeValueStore interface to the ingredient.
class RecipeImpl : public Recipe, public mojo::ApplicationDelegate {
 public:
  RecipeImpl(const GURL& renderer_url,
             const std::vector<GURL>& ingredient_urls,
             const ValueStore::Map& value_store_map);
  ~RecipeImpl() override;

 private:
  // Represents a connection to an ingredient. RecipeValueStore is exposed to
  // each ingredient.
  class IngredientConnection : public mojo::InterfaceFactory<RecipeValueStore> {
   public:
    IngredientConnection(RecipeImpl* recipe, const GURL& url);
    ~IngredientConnection() override;

   private:
    // mojo::InterfaceFactory<RecipeValueStore>:
    void Create(mojo::ApplicationConnection* connection,
                mojo::InterfaceRequest<RecipeValueStore> request) override;

    RecipeImpl* recipe_;

    scoped_ptr<RecipeValueStoreImpl> value_store_impl_;

    scoped_ptr<mojo::Binding<RecipeValueStore>> binding_;

    DISALLOW_COPY_AND_ASSIGN(IngredientConnection);
  };

  // Recipe:
  void GetIngredients(const mojo::Callback<void(mojo::Array<IngredientPtr>)>&
                          callback) override;

  // ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override;
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override;

  const GURL renderer_url_;

  const std::vector<GURL> ingredient_urls_;

  mojo::ApplicationImpl* app_;

  ScopedVector<IngredientConnection> ingredient_connections_;

  ValueStore value_store_;

  DISALLOW_COPY_AND_ASSIGN(RecipeImpl);
};

}  // recipe_handler
}  // recipes

#endif  //  EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_IMPL_H_
