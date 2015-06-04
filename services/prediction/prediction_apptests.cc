// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To test, run "out/Debug//mojo_shell mojo:prediction_apptests"

#include "base/bind.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_test_base.h"
#include "mojo/services/prediction/public/interfaces/prediction.mojom.h"

namespace prediction {

void GetPredictionListAndEnd(std::vector<std::string>* output_list,
                             const mojo::Array<mojo::String>& input_list) {
  *output_list = input_list.To<std::vector<std::string>>();
  base::MessageLoop::current()->Quit();
}

class PredictionApptest : public mojo::test::ApplicationTestBase {
 public:
  PredictionApptest() : ApplicationTestBase() {}

  ~PredictionApptest() override {}

  void SetUp() override {
    mojo::test::ApplicationTestBase::SetUp();
    application_impl()->ConnectToService("mojo:prediction_service",
                                         &prediction_);
  }

  void SetSettingsClient(bool correction,
                         bool offensive,
                         bool space_aware_gesture) {
    SettingsPtr settings = Settings::New();
    settings->correction_enabled = correction;
    settings->block_potentially_offensive = offensive;
    settings->space_aware_gesture_enabled = space_aware_gesture;
    prediction_->SetSettings(settings.Pass());
  }

  std::vector<std::string> GetPredictionListClient(
      const mojo::Array<mojo::String>& prev_words,
      const mojo::String& cur_word) {
    PredictionInfoPtr prediction_info = PredictionInfo::New();
    prediction_info->previous_words = prev_words.Clone().Pass();
    prediction_info->current_word = cur_word;

    std::vector<std::string> prediction_list;
    prediction_->GetPredictionList(
        prediction_info.Pass(),
        base::Bind(&GetPredictionListAndEnd, &prediction_list));
    base::MessageLoop::current()->Run();
    return prediction_list;
  }

 private:
  PredictionServicePtr prediction_;

  DISALLOW_COPY_AND_ASSIGN(PredictionApptest);
};

TEST_F(PredictionApptest, PredictCat) {
  SetSettingsClient(true, true, true);
  mojo::Array<mojo::String> prev_words;
  prev_words.push_back("dog");
  std::string prediction_cat = GetPredictionListClient(prev_words, "d")[0];
  EXPECT_EQ(prediction_cat, "cat");
}

}  // namespace prediction
