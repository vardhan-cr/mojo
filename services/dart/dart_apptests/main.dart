// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:apptest/apptest.dart';
import 'package:dart/test/echo_service.mojom.dart';

import 'echo_apptests.dart' as echo;
import 'pingpong_apptests.dart' as pingpong;
import 'io_http_apptests.dart' as io_http;
import 'io_internet_address_apptests.dart' as io_internet_address;

main(List args) {
  runAppTests(args[0], [echo.echoApptests,
                        io_internet_address.tests,
                        io_http.tests,
                        pingpong.pingpongApptests]);
}
