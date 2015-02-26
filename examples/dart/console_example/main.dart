// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'mojo:application';
import 'mojo:bindings';
import 'mojo:core';

import 'package:mojo/services/console/public/interfaces/console.mojom.dart';

class ConsoleApplication extends Application {
  ConsoleProxy _console;

  ConsoleApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    _console = new ConsoleProxy.unbound();
    connectToService("mojo:console", _console);
    run();
  }

  run() async {
    var result = await _console.ptr.readLine();
    await _console.ptr.printLines([result.line]);

    _console.close();
    close();
  }
}

main(List args) {
  var appHandle = new MojoHandle(args[0]);
  new ConsoleApplication.fromHandle(appHandle);
}
