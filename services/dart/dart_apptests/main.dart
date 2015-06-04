// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'package:apptest/apptest.dart';
import 'package:mojom/dart/test/echo_service.mojom.dart';

import 'connect_to_loader_apptests.dart' as connect_to_loader_apptests;
import 'echo_apptests.dart' as echo;
import 'pingpong_apptests.dart' as pingpong;
import 'io_http_apptests.dart' as io_http;
import 'io_internet_address_apptests.dart' as io_internet_address;
import 'versioning_apptests.dart' as versioning;

Future<int> main(List args) async {
  final tests = [
    connect_to_loader_apptests.connectToLoaderApptests,
    echo.echoApptests,
    io_internet_address.tests,
    io_http.tests,
    pingpong.pingpongApptests,
    versioning.tests
  ];
  var exitCode = await runAppTests(args[0], tests);
  return exitCode;
}
