// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_DART_EMBEDDER_DART_STATE_H_
#define MOJO_DART_EMBEDDER_DART_STATE_H_

#include <set>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "mojo/public/c/system/types.h"
#include "tonic/dart_library_provider.h"
#include "tonic/dart_state.h"

namespace mojo {
namespace dart {

struct IsolateCallbacks {
  base::Callback<Dart_Isolate(const char*,const char*,const char*,void*,char**)>
      create;
  base::Callback<void(void*)> shutdown;
  base::Callback<void(Dart_Handle)> exception;
};

// State associated with an isolate (retrieved via |Dart_CurrentIsolateData|).
class MojoDartState : public tonic::DartState {
 public:
  MojoDartState(void* application_data,
                bool strict_compilation,
                IsolateCallbacks callbacks,
                std::string script,
                std::string script_uri,
                std::string package_root)
      : application_data_(application_data),
        strict_compilation_(strict_compilation),
        callbacks_(callbacks),
        script_(script),
        script_uri_(script_uri),
        package_root_(package_root),
        library_provider_(nullptr) {
  }

  void* application_data() const { return application_data_; }
  bool strict_compilation() const { return strict_compilation_; }
  const IsolateCallbacks& callbacks() const { return callbacks_; }
  const std::string& script() const { return script_; }
  const std::string& script_uri() const { return script_uri_; }
  const std::string& package_root() const { return package_root_; }
  std::set<MojoHandle>& unclosed_handles() {
    return unclosed_handles_;
  }

  const std::set<MojoHandle>& unclosed_handles() const {
    return unclosed_handles_;
  }


  void set_library_provider(tonic::DartLibraryProvider* library_provider) {
    library_provider_.reset(library_provider);
    DCHECK(library_provider_.get() == library_provider);
  }

  tonic::DartLibraryProvider* library_provider() const {
    return library_provider_.get();
  }

  static MojoDartState* From(Dart_Isolate isolate) {
    return reinterpret_cast<MojoDartState*>(DartState::From(isolate));
  }

  static MojoDartState* Current() {
    return reinterpret_cast<MojoDartState*>(DartState::Current());
  }

  static MojoDartState* Cast(void* data) {
    return reinterpret_cast<MojoDartState*>(data);
  }

 private:
  void* application_data_;
  bool strict_compilation_;
  IsolateCallbacks callbacks_;
  std::string script_;
  std::string script_uri_;
  std::string package_root_;
  std::set<MojoHandle> unclosed_handles_;
  std::unique_ptr<tonic::DartLibraryProvider> library_provider_;
};

}  // namespace dart
}  // namespace mojo

#endif  // MOJO_DART_EMBEDDER_DART_STATE_H_