// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:mojo/services/console/public/interfaces/console.mojom.dart';

class ConsoleApplication extends Application {
  ConsoleProxy _proxy;

  ConsoleApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args) {
    _proxy = new ConsoleProxy.unbound();
    connectToService("mojo:console", _proxy);
    run();
  }

  run() async {
    var result = await _proxy.readLine();
    await _proxy.printLines([result.line]);

    _proxy.close();
    close();
  }
}

main(List args) {
  var appHandle = new MojoHandle(args[0]);
  var consoleApplication = new ConsoleApplication.fromHandle(appHandle);
  consoleApplication.listen();
}
