// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "dart/runtime/include/dart_api.h"
#include "dart/runtime/include/dart_debugger_api.h"
#include "dart/runtime/include/dart_native_api.h"
#include "mojo/dart/embedder/dart_debugger.h"

namespace mojo {
namespace dart {

void DartDebuggerIsolate::MessageLoop() {
  MonitorLocker ml(&monitor_);
  // Request notification on isolate messages.  This allows us to
  // respond to vm service messages while at breakpoint.
  Dart_SetMessageNotifyCallback(DartDebugger::NotifyIsolate);
  while (true) {
    // Handle all available vm service messages, up to a resume
    // request.
    bool resume = false;
    while (!resume && Dart_HasServiceMessages()) {
      monitor_.Exit();
      resume = Dart_HandleServiceMessages();
      monitor_.Enter();
    }
    if (resume) {
      break;
    }
    ml.Wait();
  }
  Dart_SetMessageNotifyCallback(nullptr);
}

void DartDebugger::BptResolvedHandler(Dart_IsolateId isolate_id,
                                      intptr_t bp_id,
                                      const Dart_CodeLocation& location) {
  // Nothing to do here. Service event is dispatched to let Observatory know
  // that a breakpoint was resolved.
}

void DartDebugger::PausedEventHandler(Dart_IsolateId isolate_id,
                                      intptr_t bp_id,
                                      const Dart_CodeLocation& loc) {
  Dart_EnterScope();
  DartDebuggerIsolate* debugger_isolate =
      FindIsolateById(isolate_id);
  CHECK(debugger_isolate != nullptr);
  debugger_isolate->MessageLoop();
  Dart_ExitScope();
}

void DartDebugger::ExceptionThrownHandler(Dart_IsolateId isolate_id,
                                          Dart_Handle exception,
                                          Dart_StackTrace stack_trace) {
  Dart_EnterScope();
  DartDebuggerIsolate* debugger_isolate =
      FindIsolateById(isolate_id);
  CHECK(debugger_isolate != nullptr);
  debugger_isolate->MessageLoop();
  Dart_ExitScope();
}

void DartDebugger::IsolateEventHandler(Dart_IsolateId isolate_id,
                                       Dart_IsolateEvent kind) {
  Dart_EnterScope();
  if (kind == Dart_IsolateEvent::kCreated) {
    AddIsolate(isolate_id);
  } else {
    DartDebuggerIsolate* debugger_isolate =
        FindIsolateById(isolate_id);
    CHECK(debugger_isolate != nullptr);
    if (kind == Dart_IsolateEvent::kInterrupted) {
      debugger_isolate->MessageLoop();
    } else {
      CHECK(kind == Dart_IsolateEvent::kShutdown);
      RemoveIsolate(isolate_id);
    }
  }
  Dart_ExitScope();
}

void DartDebugger::NotifyIsolate(Dart_Isolate isolate) {
  base::AutoLock al(*lock_);
  Dart_IsolateId isolate_id = Dart_GetIsolateId(isolate);
  DartDebuggerIsolate* debugger_isolate =
      FindIsolateByIdLocked(isolate_id);
  if (debugger_isolate != nullptr) {
    debugger_isolate->Notify();
  }
}

void DartDebugger::InitDebugger() {
  Dart_SetIsolateEventHandler(IsolateEventHandler);
  Dart_SetPausedEventHandler(PausedEventHandler);
  Dart_SetBreakpointResolvedHandler(BptResolvedHandler);
  Dart_SetExceptionThrownHandler(ExceptionThrownHandler);
  lock_ = new base::Lock();
}

DartDebuggerIsolate* DartDebugger::FindIsolateById(Dart_IsolateId id) {
  base::AutoLock al(*lock_);
  return FindIsolateByIdLocked(id);
}

DartDebuggerIsolate* DartDebugger::FindIsolateByIdLocked(
      Dart_IsolateId id) {
  lock_->AssertAcquired();
  for (size_t i = 0; i < isolates_.size(); i++) {
    DartDebuggerIsolate* isolate = isolates_[i];
    if (id == isolate->id()) {
      return isolate;
    }
  }
  return nullptr;
}

DartDebuggerIsolate* DartDebugger::AddIsolate(Dart_IsolateId id) {
  base::AutoLock al(*lock_);
  CHECK(FindIsolateByIdLocked(id) == nullptr);
  DartDebuggerIsolate* debugger_isolate =
      new DartDebuggerIsolate(id);
  isolates_.push_back(debugger_isolate);
  return debugger_isolate;
}

void DartDebugger::RemoveIsolate(Dart_IsolateId id) {
  base::AutoLock al(*lock_);
  for (size_t i = 0; i < isolates_.size(); i++) {
    DartDebuggerIsolate* isolate = isolates_[i];
    if (id == isolate->id()) {
      isolates_.erase(isolates_.begin() + i);
      return;
    }
  }
  NOTREACHED();
}

base::Lock* DartDebugger::lock_ = nullptr;
std::vector<DartDebuggerIsolate*> DartDebugger::isolates_;

}  // namespace apps
}  // namespace mojo
