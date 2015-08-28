// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "apps/benchmark/event.h"
#include "apps/benchmark/measurements.h"
#include "apps/benchmark/run_args.h"
#include "apps/benchmark/trace_collector_client.h"
#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/scoped_ptr.h"
#include "base/strings/string_util.h"
#include "base/time/time.h"
#include "base/trace_event/trace_event.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/services/tracing/public/interfaces/tracing.mojom.h"

namespace benchmark {
namespace {

class BenchmarkApp : public mojo::ApplicationDelegate,
                     public TraceCollectorClient::Receiver {
 public:
  BenchmarkApp() {}
  ~BenchmarkApp() override {}

  // mojo:ApplicationDelegate:
  void Initialize(mojo::ApplicationImpl* app) override {
    // Parse command-line arguments.
    if (!GetRunArgs(app->args(), &args_)) {
      LOG(ERROR) << "Failed to parse the input arguments.";
      mojo::ApplicationImpl::Terminate();
      return;
    }

    // Calculate a list of trace categories we want to collect: a union of all
    // categories targeted in measurements.
    std::set<std::string> category_set;
    for (const Measurement& measurement : args_.measurements) {
      category_set.insert(measurement.target_event.category);
    }
    std::vector<std::string> unique_categories(category_set.begin(),
                                               category_set.end());
    std::string categories_str = JoinString(unique_categories, ',');

    // Connect to trace collector, which will fetch the trace events produced by
    // the app being benchmarked.
    tracing::TraceCollectorPtr trace_collector;
    app->ConnectToService("mojo:tracing", &trace_collector);
    trace_collector_client_.reset(
        new TraceCollectorClient(this, trace_collector.Pass()));
    trace_collector_client_->Start(categories_str);

    // Record the time origin for measurements just before connecting to the app
    // being benchmarked.
    time_origin_ = base::TimeTicks::FromInternalValue(MojoGetTimeTicksNow());
    traced_app_connection_ = app->ConnectToApplication(args_.app);

    // Post task to stop tracing when the time is up.
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&BenchmarkApp::StopTracing, base::Unretained(this)),
        args_.duration);
  }

  void StopTracing() {
    // Request the trace collector to send back the data. When the data is ready
    // we will be called at OnTraceCollected().
    trace_collector_client_->Stop();
  }

  // TraceCollectorClient::Receiver:
  void OnTraceCollected(std::string trace_data) override {
    // Parse trace events.
    std::vector<Event> events;
    if (!GetEvents(trace_data, &events)) {
      LOG(ERROR) << "Failed to parse the trace data";
      mojo::ApplicationImpl::Terminate();
      return;
    }

    // Calculate and print the results.
    bool succeeded = true;
    Measurements measurements(events, time_origin_);
    for (const Measurement& measurement : args_.measurements) {
      double result = measurements.Measure(measurement);
      if (result >= 0.0) {
        printf("measurement: %s %lf\n", measurement.spec.c_str(), result);
      } else {
        succeeded = false;
        printf("measurement: %s FAILED\n", measurement.spec.c_str());
      }
    }

    // Scripts that run benchmarks can pick this up as a signal that the run
    // succeeded, as shell exit code is 0 even if an app exits early due to an
    // error.
    if (succeeded) {
      printf("benchmark succeeded\n");
    } else {
      printf("some measurements failed\n");
    }
    mojo::ApplicationImpl::Terminate();
  }

 private:
  RunArgs args_;
  mojo::ApplicationConnection* traced_app_connection_;
  scoped_ptr<TraceCollectorClient> trace_collector_client_;
  base::TimeTicks time_origin_;

  DISALLOW_COPY_AND_ASSIGN(BenchmarkApp);
};
}  // namespace
}  // namespace benchmark

MojoResult MojoMain(MojoHandle application_request) {
  mojo::ApplicationRunnerChromium runner(new benchmark::BenchmarkApp);
  auto ret = runner.Run(application_request);
  fflush(nullptr);
  return ret;
}
