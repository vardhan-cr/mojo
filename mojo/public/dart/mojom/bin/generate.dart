// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/// This script generates mojo bindings for the .mojo files listed in a Dart
/// package's .mojoms file.

import 'package:mojom/generate.dart';

main(List<String> arguments) {
  MojomGenerator.singleMain(arguments);
}
