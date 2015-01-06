// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/recipes/recipe_handler/recipe_impl.h"

#include "examples/recipes/recipe_handler/recipe_value_store_impl.h"
#include "mojo/public/cpp/application/application_impl.h"

namespace recipes {
namespace recipe_handler {

// IngredientConnection --------------------------------------------------------

RecipeImpl::IngredientConnection::IngredientConnection(RecipeImpl* recipe,
                                                       const GURL& url)
    : recipe_(recipe) {
  recipe_->app_->ConnectToApplication(url.spec())->AddService(this);
}

RecipeImpl::IngredientConnection::~IngredientConnection() {
}

void RecipeImpl::IngredientConnection::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<RecipeValueStore> request) {
  if (value_store_impl_.get())
    return;

  value_store_impl_.reset(new RecipeValueStoreImpl(&recipe_->value_store_));
  binding_.reset(new mojo::Binding<RecipeValueStore>(value_store_impl_.get(),
                                                     request.Pass()));
}

// RecipeImpl ------------------------------------------------------------------

RecipeImpl::RecipeImpl(const GURL& renderer_url,
                       const std::vector<GURL>& ingredient_urls,
                       const ValueStore::Map& value_store_map)
    : renderer_url_(renderer_url),
      ingredient_urls_(ingredient_urls),
      app_(nullptr),
      value_store_(value_store_map) {
}

RecipeImpl::~RecipeImpl() {
  // IngredientConnections end up installing observers on |value_store_|. As
  // such, we need to destroy the IngredientConnections first.
  ingredient_connections_.clear();
}

void RecipeImpl::GetIngredients(
    const mojo::Callback<void(mojo::Array<IngredientPtr>)>& callback) {
  mojo::Array<IngredientPtr> results;
  results.resize(ingredient_urls_.size());
  for (size_t i = 0; i < ingredient_urls_.size(); ++i) {
    results[i] = Ingredient::New();
    results[i]->url = ingredient_urls_[i].spec();
  }
  callback.Run(results.Pass());
}

void RecipeImpl::Initialize(mojo::ApplicationImpl* app) {
  app_ = app;
  for (const GURL& url : ingredient_urls_)
    ingredient_connections_.push_back(new IngredientConnection(this, url));
}

bool RecipeImpl::ConfigureIncomingConnection(
    mojo::ApplicationConnection* connection) {
  // TODO(sky): expose Recipe by way of InterfaceFactory.
  return true;
}

}  // namespace recipe_handler
}  // namespace recipes
