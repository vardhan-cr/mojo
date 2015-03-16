// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:mojo.core';

main() {
  try {
    throw 'exception';
  } catch (exception, stackTrace) {
    assert(exception == 'exception');
    assert(stackTrace != null);
  }
}
