// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Make sure that we are generating valid Dart code for all mojom interface
// tests.
// vmoptions: --compile_all

import 'dart:async';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:mojo/dart/testing/expect.dart';
import 'package:mojo/public/dart/application.dart';
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';
import 'package:mojo/application.mojom.dart';
import 'package:mojo/service_provider.mojom.dart';
import 'package:mojo/shell.mojom.dart';
import 'package:math/math_calculator.mojom.dart';
import 'package:no_module.mojom.dart';
import 'package:mojo/test/rect.mojom.dart';
import 'package:regression_tests/regression_tests.mojom.dart';
import 'package:sample/sample_factory.mojom.dart';
import 'package:imported/sample_import2.mojom.dart';
import 'package:imported/sample_import.mojom.dart';
import 'package:sample/sample_interfaces.mojom.dart';
import 'package:sample/sample_service.mojom.dart';
import 'package:mojo/test/serialization_test_structs.mojom.dart';
import 'package:mojo/test/test_structs.mojom.dart';
import 'package:mojo/test/validation_test_interfaces.mojom.dart';

int main() {
  return 0;
}
