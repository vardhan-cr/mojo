// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "base/macros.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/interface_factory.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "mojo/services/console/public/interfaces/console.mojom.h"

namespace mojo {

class ConsoleImpl : public Console {
 public:
  ConsoleImpl(const std::string& app_name, InterfaceRequest<Console> request)
      : app_name_(app_name), binding_(this, request.Pass()) {}
  ~ConsoleImpl() override {}

  void ReadLine(const Callback<void(bool, String)>& callback) override {
    std::cout << "[" << app_name_ << "]> ";
    std::cout.flush();

    std::string line;
    std::getline(std::cin, line);

    // This will send an empty string on eof.
    String return_value = String::From(line);
    bool is_good = std::cin.good();
    if (!is_good)
      return_value = String::From("");

    callback.Run(is_good, return_value);
  }

  void PrintLines(Array<mojo::String> data,
                  const Callback<void(bool)>& callback) override {
    for (size_t i = 0; i < data.size(); ++i)
      std::cout << "[" << app_name_ << "] " << data[i].get() << std::endl;

    callback.Run(true);
  }

 private:
  const std::string app_name_;
  StrongBinding<Console> binding_;

  DISALLOW_COPY_AND_ASSIGN(ConsoleImpl);
};

class ConsoleDelegate : public ApplicationDelegate,
                        public InterfaceFactory<Console> {
 public:
  ConsoleDelegate() {}
  ~ConsoleDelegate() override {}

  // ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(ApplicationConnection* connection) override {
    connection->AddService(this);
    return true;
  }

  // InterfaceFactory<Console> implementation.
  void Create(ApplicationConnection* connection,
              InterfaceRequest<Console> request) override {
    new ConsoleImpl(connection->GetRemoteApplicationURL(), request.Pass());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ConsoleDelegate);
};

}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new mojo::ConsoleDelegate);
  return runner.Run(application_request);
}
