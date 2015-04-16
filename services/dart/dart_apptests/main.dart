// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'package:apptest/apptest.dart';
import 'package:dart/test/echo_service.mojom.dart';

import 'echo_apptests.dart' as echo;
import 'pingpong_apptests.dart' as pingpong;
import 'io_apptests.dart' as io;

main(List args) {
  runAppTests(args[0], [echo.echoApptests,
                        io.ioApptests,
                        pingpong.pingpongApptests]);
}
