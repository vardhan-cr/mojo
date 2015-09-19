// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_KEYBOARD_NATIVE_PREDICTOR_H_
#define SERVICES_KEYBOARD_NATIVE_PREDICTOR_H_

#include <vector>

#include "base/callback.h"
#include "mojo/public/cpp/application/connect.h"
#include "mojo/public/interfaces/application/shell.mojom.h"
#include "mojo/services/prediction/public/interfaces/prediction.mojom.h"
#include "services/keyboard_native/key_layout.h"

namespace keyboard {

class Predictor {
 public:
  Predictor(mojo::Shell* shell);
  ~Predictor();

  void SetSuggestionKeys(std::vector<KeyLayout::Key*> suggestion_keys);

  void SetUpdateCallback(base::Callback<void()> on_update_callback);

  void StoreCurWord(std::string new_word);

  int ChooseSuggestedWord(std::string suggested);

  void DeleteCharInCurWord();

 private:
  void ShowEmptySuggestion();

  void GetSuggestion();

  void GetPredictionListAndEnd(const mojo::Array<mojo::String>& input_list);

  prediction::PredictionServicePtr prediction_service_impl_;
  std::vector<KeyLayout::Key*> suggestion_keys_;
  std::string current_word_;
  std::vector<std::string> previous_words_;
  base::Callback<void()> on_update_callback_;

  DISALLOW_COPY_AND_ASSIGN(Predictor);
};

}  // namespace keyboard

#endif  // SERVICES_KEYBOARD_NATIVE_PREDICTOR_H_
