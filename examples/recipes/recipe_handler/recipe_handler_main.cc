// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/json/json_reader.h"
#include "base/values.h"
#include "examples/recipes/recipe_handler/recipe_factory.h"
#include "examples/recipes/recipe_handler/recipe_impl.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/application/content_handler_factory.h"
#include "mojo/common/data_pipe_utils.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/interface_factory_impl.h"
#include "mojo/public/cpp/application/service_provider_impl.h"
#include "mojo/services/content_handler/public/interfaces/content_handler.mojom.h"

namespace recipes {
namespace recipe_handler {

class RecipeHandlerApp : public mojo::ApplicationDelegate,
                         public mojo::ContentHandlerFactory::ManagedDelegate {
 public:
  RecipeHandlerApp() : content_handler_factory_(this) {}

  ~RecipeHandlerApp() override {}

 private:
  // Overridden from ApplicationDelegate:
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService(&content_handler_factory_);
    return true;
  }

  // Overridden from ContentHandlerFactory::ManagedDelegate:
  scoped_ptr<mojo::ContentHandlerFactory::HandledApplicationHolder>
  CreateApplication(mojo::ShellPtr shell,
                    mojo::URLResponsePtr response) override {
    std::string recipe_string;
    if (!mojo::common::BlockingCopyToString(response->body.Pass(),
                                            &recipe_string)) {
      LOG(WARNING) << "failed reading recipe";
      return nullptr;
    }

    base::JSONReader reader(base::JSON_ALLOW_TRAILING_COMMAS);
    scoped_ptr<base::Value> recipe_value(reader.ReadToValue(recipe_string));
    if (!recipe_value.get()) {
      LOG(WARNING) << "failed parsing recipe: " << reader.GetErrorMessage();
      return nullptr;
    }

    if (!recipe_value->IsType(base::Value::TYPE_DICTIONARY)) {
      LOG(WARNING) << "recipe is not a dictionary";
      return nullptr;
    }

    RecipeImpl* recipe =
        CreateRecipe(*static_cast<base::DictionaryValue*>(recipe_value.get()));
    if (!recipe)
      return nullptr;

    return make_handled_factory_holder(
        new mojo::ApplicationImpl(recipe, shell.PassMessagePipe()));
  }

  mojo::ContentHandlerFactory content_handler_factory_;

  DISALLOW_COPY_AND_ASSIGN(RecipeHandlerApp);
};

}  // namespace recipe_handler
}  // namespace recipes

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new recipes::recipe_handler::RecipeHandlerApp());
  return runner.Run(shell_handle);
}
