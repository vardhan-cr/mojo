// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "examples/bank_app/bank.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/utility/run_loop.h"
#include "mojo/services/vanadium/security/public/interfaces/principal.mojom.h"

namespace examples {

class LoginHandler {
 public:
  void Run(const vanadium::BlessingPtr& b) const {
    std::string user;
    if (b) {
      for (size_t i = 0; i < b->chain.size(); i++) {
        user += vanadium::ChainSeparator;
        user += b->chain[i]->extension;
      }
    }
    if (!user.empty()) {
      MOJO_LOG(INFO) << "Welcome: " << user;
    }
  }
};

class BankCustomer : public mojo::ApplicationDelegate {
 public:
  void Initialize(mojo::ApplicationImpl* app) override {
    // Get user login credentials
    app->ConnectToService("mojo:principal_service", &login_service_);
    login_service_->Login(LoginHandler());
    // Check and see whether we got a valid user blessing.
    if (!login_service_.WaitForIncomingResponse()) {
      MOJO_LOG(INFO) << "Login() to the principal service failed\n";
    }
    BankPtr bank;
    app->ConnectToService("mojo:bank", &bank);
    bank->Deposit(500/*usd*/);
    bank->Withdraw(100/*usd*/);
    auto gb_callback = [](const int32_t& balance) {
      MOJO_LOG(INFO) << "Bank balance: " << balance;
    };
    bank->GetBalance(mojo::Callback<void(const int32_t&)>(gb_callback));
    bank.WaitForIncomingResponse();
  }
  void Quit() override {
    login_service_->Logout();
  }
 private:
  vanadium::PrincipalServicePtr login_service_;
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new examples::BankCustomer);
  return runner.Run(application_request);
}
