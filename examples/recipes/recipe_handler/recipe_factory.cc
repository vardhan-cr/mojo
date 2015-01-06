// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "examples/recipes/recipe_handler/recipe_factory.h"

#include <vector>

#include "base/values.h"
#include "examples/recipes/recipe_handler/recipe_impl.h"
#include "examples/recipes/recipe_handler/value_store.h"
#include "url/gurl.h"

namespace recipes {
namespace recipe_handler {
namespace {

const char* kIngredientsKey = "ingredients";
const char* kIngredientURLKey = "url";
const char* kRendererURLKey = "renderer";
const char* kValueStoreKey = "values";

bool ExtractValueStoreMap(const base::DictionaryValue& value,
                          ValueStore::Map* map) {
  const base::DictionaryValue* values_value = nullptr;
  if (!value.GetDictionary(kValueStoreKey, &values_value)) {
    DVLOG(1) << "'values' not found";
    return false;
  }

  base::DictionaryValue::Iterator i(*values_value);
  while (!i.IsAtEnd()) {
    std::string string_value;
    if (!i.value().IsType(base::Value::TYPE_STRING) ||
        !i.value().GetAsString(&string_value)) {
      DVLOG(1) << "value for key " << i.key()
               << " is not a string, value=" << i.value();
      return false;
    }
    (*map)[i.key()] =
        std::vector<uint8_t>(string_value.begin(), string_value.end());
    i.Advance();
  }
  return true;
}

bool ExtractIngredientURLs(const base::DictionaryValue& value,
                           std::vector<GURL>* urls) {
  const base::ListValue* ingredients_value = nullptr;
  if (!value.GetList(kIngredientsKey, &ingredients_value)) {
    DVLOG(1) << "'ingredients' not found";
    return false;
  }

  for (size_t i = 0; i < ingredients_value->GetSize(); ++i) {
    const base::DictionaryValue* ingredient_value = nullptr;
    if (!ingredients_value->GetDictionary(i, &ingredient_value)) {
      DVLOG(1) << "ingredient at index " << i << " is not a dictionary";
      return false;
    }

    std::string url_value;
    if (!ingredient_value->GetString(kIngredientURLKey, &url_value)) {
      DVLOG(1) << "ingredient at index " << i << " does not have a url";
      return false;
    }

    const GURL url(url_value);
    if (url.is_empty() || !url.is_valid()) {
      DVLOG(1) << "ingredient at index " << i << " url is not valid "
               << url_value;
      return false;
    }
    urls->push_back(url);
  }
  return true;
}

bool ExtractRendererURL(const base::DictionaryValue& value, GURL* url) {
  std::string url_as_string;
  if (!value.GetString(kRendererURLKey, &url_as_string)) {
    DVLOG(1) << "need url for renderer";
    return false;
  }

  *url = GURL(url_as_string);
  if (url->is_empty() || !url->is_valid()) {
    DVLOG(1) << "renderer url is not valid " << url_as_string;
    return false;
  }
  return true;
}

}  // namespace

RecipeImpl* CreateRecipe(const base::DictionaryValue& value) {
  // We expect the following:
  // 'renderer': url of the renderer.
  // 'ingredients': list of dictionaries each having a key named 'url'.
  // 'values': dictionary of key/value pairs to expose (values must be strings).

  ValueStore::Map value_store;
  if (!ExtractValueStoreMap(value, &value_store))
    return nullptr;

  std::vector<GURL> ingredient_urls;
  if (!ExtractIngredientURLs(value, &ingredient_urls))
    return nullptr;

  GURL renderer_url;
  if (!ExtractRendererURL(value, &renderer_url))
    return nullptr;

  return new RecipeImpl(renderer_url, ingredient_urls, value_store);
}

}  // recipe_handler
}  // recipes
