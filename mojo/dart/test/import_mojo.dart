// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:mojo_bindings';
import 'dart:mojo_core';

main() {
  MojoMessagePipe pipe = new MojoMessagePipe();
  pipe.endpoints[0].handle.close();
  pipe.endpoints[1].handle.close();
}
