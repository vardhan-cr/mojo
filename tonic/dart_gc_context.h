// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_GC_CONTEXT_H_
#define TONIC_DART_GC_CONTEXT_H_

#include <unordered_map>

#include "base/macros.h"
#include "dart/runtime/include/dart_api.h"

namespace tonic {

class DartGCContext {
 public:
  DartGCContext();
  ~DartGCContext();

  Dart_WeakReferenceSet AddToSetForRoot(const void* root,
                                        Dart_WeakPersistentHandle handle);

 private:
  Dart_WeakReferenceSetBuilder builder_;
  std::unordered_map<const void*, Dart_WeakReferenceSet> references_;

  DISALLOW_COPY_AND_ASSIGN(DartGCContext);
};

}  // namespace tonic

#endif  // TONIC_DART_GC_CONTEXT_H_
