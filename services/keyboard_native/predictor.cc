// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iterator>
#include <sstream>
#include <string>

#include "base/bind.h"
#include "services/keyboard_native/predictor.h"
#include "services/keyboard_native/text_update_key.h"

namespace keyboard {

Predictor::Predictor(mojo::Shell* shell) {
  mojo::ServiceProviderPtr prediction_service_provider;
  shell->ConnectToApplication("mojo:prediction_service",
                              mojo::GetProxy(&prediction_service_provider),
                              nullptr);
  mojo::ConnectToService(prediction_service_provider.get(),
                         &prediction_service_impl_);
  suggestion_keys_.clear();
}

Predictor::~Predictor() {
}

void Predictor::SetSuggestionKeys(
    std::vector<KeyLayout::Key*> suggestion_keys) {
  size_t old_size = suggestion_keys_.size();
  size_t new_size = suggestion_keys.size();
  size_t keyloop_size = std::min(old_size, new_size);
  for (size_t i = 0; i < keyloop_size; i++) {
    if (old_size != 0) {
      static_cast<TextUpdateKey*>(suggestion_keys[i])
          ->ChangeText(suggestion_keys_[i]->ToText());
      suggestion_keys_[i] = suggestion_keys[i];
    } else {
      suggestion_keys_.push_back(suggestion_keys[i]);
    }
  }
  if (new_size < old_size) {
    suggestion_keys_.erase(suggestion_keys_.begin() + new_size,
                           suggestion_keys_.end());
  } else if (old_size < new_size) {
    suggestion_keys_.insert(suggestion_keys_.end(),
                            suggestion_keys.begin() + old_size,
                            suggestion_keys.end());
  }
}

void Predictor::SetUpdateCallback(base::Callback<void()> on_update_callback) {
  on_update_callback_ = on_update_callback;
}

void Predictor::StoreCurWord(std::string new_word) {
  if (new_word == " ") {
    previous_words_.push_back(current_word_);
    current_word_ = "";
    Predictor::ShowEmptySuggestion();
  } else {
    current_word_ += new_word;
    Predictor::GetSuggestion();
  }
}

int Predictor::ChooseSuggestedWord(std::string suggested) {
  int old_size = static_cast<int>(current_word_.size());
  // split suggested by space into a vector
  std::istringstream sug(suggested);
  std::istream_iterator<std::string> beg(sug), end;
  std::vector<std::string> sugs(beg, end);
  previous_words_.insert(previous_words_.end(), sugs.begin(), sugs.end());
  current_word_ = "";
  Predictor::ShowEmptySuggestion();
  return old_size;
}

void Predictor::DeleteCharInCurWord() {
  if (!current_word_.empty()) {
    current_word_.erase(current_word_.end() - 1);
    if (current_word_.empty()) {
      Predictor::ShowEmptySuggestion();
    } else {
      Predictor::GetSuggestion();
    }
  } else if (!previous_words_.empty()) {
    current_word_ = previous_words_.back();
    previous_words_.pop_back();
    if (!current_word_.empty())
      Predictor::GetSuggestion();
  }
}

void Predictor::ShowEmptySuggestion() {
  for (size_t i = 0; i < suggestion_keys_.size(); i++) {
    static_cast<TextUpdateKey*>(suggestion_keys_[i])->ChangeText("");
  }
  on_update_callback_.Run();
}

void Predictor::GetSuggestion() {
  prediction::PredictionInfoPtr prediction_info =
      prediction::PredictionInfo::New();
  // we are not using bigram atm
  prediction_info->previous_words =
      mojo::Array<prediction::PrevWordInfoPtr>::New(0).Pass();
  prediction_info->current_word = mojo::String(current_word_);

  prediction_service_impl_->GetPredictionList(
      prediction_info.Pass(),
      base::Bind(&Predictor::GetPredictionListAndEnd, base::Unretained(this)));
}

void Predictor::GetPredictionListAndEnd(
    const mojo::Array<mojo::String>& input_list) {
  for (size_t i = 0; i < suggestion_keys_.size(); i++) {
    std::string change_text;
    if (i < input_list.size()) {
      change_text = std::string(input_list[i].data());
    } else {
      change_text = "";
    }
    static_cast<TextUpdateKey*>(suggestion_keys_[i])->ChangeText(change_text);
  }
  on_update_callback_.Run();
}

}  // namespace keyboard
