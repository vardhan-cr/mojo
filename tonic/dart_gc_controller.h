// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TONIC_DART_GC_CONTROLLER_H_
#define TONIC_DART_GC_CONTROLLER_H_

namespace tonic {

void DartGCPrologue();
void DartGCEpilogue();

}  // namespace tonic

#endif  // TONIC_DART_GC_CONTROLLER_H_
