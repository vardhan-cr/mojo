// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This app is run by examples/dart/hello_world/hello.

import 'dart:async';

import 'package:mojo/public/dart/application.dart';
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';

class World extends Application {
  World.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    print("$url World");
    closeApplication();
  }

  Future closeApplication() async {
    await close();
    assert(MojoHandle.reportLeakedHandles());
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  new World.fromHandle(appHandle);
}
