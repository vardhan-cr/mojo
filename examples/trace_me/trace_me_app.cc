// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/tracing_impl.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner.h"

// This is an example app that uses implementation of tracing from mojo/common
// to participate in the tracing ecosystem.
//
// To collect a startup trace, run:
//   `mojo_run mojo:trace_me --trace-startup [--android]
//
// You can also use `mojo_debug` to start and stop tracing interactively.

namespace examples {

static const base::TimeDelta kDoWorkDelay =
    base::TimeDelta::FromMilliseconds(100);
static const base::TimeDelta kSleepTime =
    base::TimeDelta::FromMilliseconds(100);

void NestedFunction() {
  TRACE_EVENT0("trace_me", "NestedFunction");
  base::PlatformThread::Sleep(kSleepTime);
}

void DoWork() {
  TRACE_EVENT0("trace_me", "DoWork");
  base::PlatformThread::Sleep(kSleepTime);
  NestedFunction();
  base::PlatformThread::Sleep(kSleepTime);
  base::MessageLoop::current()->PostDelayedTask(FROM_HERE, base::Bind(&DoWork),
                                                kDoWorkDelay);
}

class TraceMeApp : public mojo::ApplicationDelegate {
 public:
  TraceMeApp() {}
  ~TraceMeApp() override {}

  // mojo:ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    TRACE_EVENT0("trace_me", "TraceMeApp::Initialize");

    // Allow TracingImpl to connect to the central tracing service, advertising
    // its ability to provide trace events.
    tracing_.Initialize(app);

    base::MessageLoop::current()->PostTask(FROM_HERE, base::Bind(&DoWork));
  }

 private:
  mojo::TracingImpl tracing_;
  DISALLOW_COPY_AND_ASSIGN(TraceMeApp);
};

}  // namespace examples

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new examples::TraceMeApp);
  return runner.Run(application_request);
}
