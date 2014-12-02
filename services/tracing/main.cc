// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "mojo/application/application_runner_chromium.h"
#include "mojo/common/weak_binding_set.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "services/tracing/trace_data_sink.h"
#include "services/tracing/tracing.mojom.h"

namespace tracing {

class TracingApp : public mojo::ApplicationDelegate,
                   public mojo::InterfaceFactory<TraceCoordinator>,
                   public tracing::TraceCoordinator,
                   public mojo::InterfaceFactory<TraceDataCollector>,
                   public tracing::TraceDataCollector {
 public:
  TracingApp() {}
  ~TracingApp() override {}

 private:
  // mojo::ApplicationDelegate implementation.
  bool ConfigureIncomingConnection(
      mojo::ApplicationConnection* connection) override {
    connection->AddService<TraceCoordinator>(this);
    connection->AddService<TraceDataCollector>(this);
    return true;
  }

  // mojo::InterfaceFactory<TraceCoordinator> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<TraceCoordinator> request) override {
    coordinator_bindings_.AddBinding(this, request.Pass());
  }

  // mojo::InterfaceFactory<TraceDataCollector> implementation.
  void Create(mojo::ApplicationConnection* connection,
              mojo::InterfaceRequest<TraceDataCollector> request) override {
    collector_bindings_.AddBinding(this, request.Pass());
  }

  // tracing::TraceCoordinator implementation.
  void Start(const mojo::String& base_name,
             const mojo::String& categories) override {
    base::FilePath base_name_path =
        base::FilePath::FromUTF8Unsafe(base_name.To<std::string>());
    sink_.reset(new TraceDataSink(base_name_path));
    collector_bindings_.ForAllBindings([categories](
        TraceController* controller) { controller->StartTracing(categories); });
  }
  void StopAndFlush() override {
    collector_bindings_.ForAllBindings(
        [](TraceController* controller) { controller->StopTracing(); });

    // TODO: We really should keep track of how many connections we have here
    // and flush + reset the sink after we receive a EndTracing or a detect a
    // pipe closure on all pipes.
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&TraceDataSink::Flush, base::Unretained(sink_.get())),
        base::TimeDelta::FromSeconds(1));
  }

  // tracing::TraceDataCollector implementation.
  void DataCollected(const mojo::String& json) override {
    if (sink_)
      sink_->AddChunk(json.To<std::string>());
  }
  // tracing::TraceDataCollector implementation.
  void EndTracing() override {}

  scoped_ptr<TraceDataSink> sink_;
  mojo::WeakBindingSet<TraceDataCollector> collector_bindings_;
  mojo::WeakBindingSet<TraceCoordinator> coordinator_bindings_;

  DISALLOW_COPY_AND_ASSIGN(TracingApp);
};

}  // namespace tracing

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new tracing::TracingApp);
  return runner.Run(shell_handle);
}
