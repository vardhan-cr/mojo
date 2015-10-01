// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/dart/dart_tracing.h"

#include "dart/runtime/include/dart_tools_api.h"
#include "mojo/public/cpp/application/application_impl.h"

namespace dart {

DartTraceProvider::DartTraceProvider()
    : binding_(this) {
}

DartTraceProvider::~DartTraceProvider() {
}

void DartTraceProvider::Bind(
    mojo::InterfaceRequest<tracing::TraceProvider> request) {
  if (!binding_.is_bound()) {
    binding_.Bind(request.Pass());
  } else {
    LOG(ERROR) << "Cannot accept two connections to TraceProvider.";
  }
}

// tracing::TraceProvider implementation:
void DartTraceProvider::StartTracing(const mojo::String& categories,
                                     tracing::TraceRecorderPtr recorder) {
  DCHECK(!recorder_.get());
  recorder_ = recorder.Pass();
  // TODO(johnmccutchan): Respect |categories|.
  Dart_GlobalTimelineSetRecordedStreams(DART_TIMELINE_STREAM_ALL |
                                        DART_TIMELINE_STREAM_VM);
}

static void AppendStreamConsumer(Dart_StreamConsumer_State state,
                                 const char* stream_name,
                                 const uint8_t* buffer,
                                 intptr_t buffer_length,
                                 void* user_data) {
  if (state == Dart_StreamConsumer_kFinish) {
    return;
  }
  std::vector<uint8_t>* data =
      reinterpret_cast<std::vector<uint8_t>*>(user_data);
  DCHECK(data);
  if (state == Dart_StreamConsumer_kStart) {
    data->clear();
    return;
  }
  DCHECK_EQ(state, Dart_StreamConsumer_kData);
  // Append data.
  data->insert(data->end(), buffer, buffer + buffer_length);
}

// tracing::TraceProvider implementation:
void DartTraceProvider::StopTracing() {
  DCHECK(recorder_);
  Dart_GlobalTimelineSetRecordedStreams(DART_TIMELINE_STREAM_DISABLE);
  std::vector<uint8_t> data;
  bool got_trace = Dart_GlobalTimelineGetTrace(AppendStreamConsumer, &data);
  if (got_trace) {
    recorder_->Record(reinterpret_cast<char*>(data.data()));
  }
  recorder_.reset();
}

DartTracingImpl::DartTracingImpl() {
}

DartTracingImpl::~DartTracingImpl() {
}

void DartTracingImpl::Initialize(mojo::ApplicationImpl* app) {
  auto connection = app->ConnectToApplication("mojo:tracing");
  connection->AddService(this);
}

void DartTracingImpl::Create(
    mojo::ApplicationConnection* connection,
    mojo::InterfaceRequest<tracing::TraceProvider> request) {
  provider_impl_.Bind(request.Pass());
}

}  // namespace dart
