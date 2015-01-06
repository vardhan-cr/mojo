// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_FACTORY_H_
#define EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_FACTORY_H_

namespace base {
class DictionaryValue;
}

namespace recipes {
namespace recipe_handler {

class RecipeImpl;

// Creates a RecipeImpl from the supplied argument. Returns null if |value| does
// not contain expected data.
// Caller owns the return value.
RecipeImpl* CreateRecipe(const base::DictionaryValue& value);

}  // recipe_handler
}  // recipes

#endif  //  EXAMPLES_RECIPES_RECIPE_HANDLER_RECIPE_FACTORY_H_
