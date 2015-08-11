// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'package:apptest/apptest.dart';
import 'package:mojom/dart/test/echo_service.mojom.dart';

import 'src/connect_to_loader_apptests.dart' as connect_to_loader_apptests;
import 'src/echo_apptests.dart' as echo;
import 'src/pingpong_apptests.dart' as pingpong;
import 'src/io_http_apptests.dart' as io_http;
import 'src/io_internet_address_apptests.dart' as io_internet_address;
import 'src/versioning_apptests.dart' as versioning;

main(List args) {
  final tests = [
    connect_to_loader_apptests.connectToLoaderApptests,
    echo.echoApptests,
    io_internet_address.tests,
    io_http.tests,
    pingpong.pingpongApptests,
    versioning.tests
  ];
  runAppTests(args[0], tests);
}
