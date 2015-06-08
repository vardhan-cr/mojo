// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_EDK_SYSTEM_PROCESS_IDENTIFIER_H_
#define MOJO_EDK_SYSTEM_PROCESS_IDENTIFIER_H_

#include <stdint.h>

namespace mojo {
namespace system {

// Identifiers for processes (note that these are not OS process IDs):
using ProcessIdentifier = uint64_t;
const ProcessIdentifier kInvalidProcessIdentifier = 0;

}  // namespace system
}  // namespace mojo

#endif  // MOJO_EDK_SYSTEM_PROCESS_IDENTIFIER_H_
