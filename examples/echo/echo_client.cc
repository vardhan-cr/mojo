// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>

#include "base/logging.h"
#include "examples/echo/echo.mojom.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"
#include "mojo/public/cpp/utility/run_loop.h"

namespace mojo {
namespace examples {

class ResponsePrinter {
 public:
  void Run(const String& value) const {
    LOG(INFO) << "***** Response: " << value.get().c_str();
    RunLoop::current()->Quit();  // All done!
  }
};

class EchoClientDelegate : public ApplicationDelegate {
 public:
  void Initialize(ApplicationImpl* app) override {
    app->ConnectToService("mojo:echo_server", &echo_);

    echo_->EchoString("hello world", ResponsePrinter());
  }

 private:
  EchoPtr echo_;
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunner runner(new mojo::examples::EchoClientDelegate);
  return runner.Run(application_request);
}
