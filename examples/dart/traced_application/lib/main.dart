#!mojo mojo:dart_content_handler
// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This example application illustrates how to use Mojo Tracing from Dart code.
//
// To run this app:
// mojo/devtools/common/mojo_run --trace-startup \
//   https://core.mojoapps.io/examples/dart/traced_application/lib/main.dart
//
// This will produce a file called |mojo_shell.trace| that may be loaded
// by Chrome's about:tracing.

import 'dart:async';
import 'dart:io';

import 'package:common/tracing_helper.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

class TestUsingTracingApp extends Application {
  TracingHelper _tracing;

  TestUsingTracingApp.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    // This sets up a connection between this application and the Mojo
    // tracing service.
    _tracing = new TracingHelper(this, "example_traced_application");

    // Now we schedule some random work just so we have something to trace.
    new Timer.periodic(new Duration(seconds: 1), (t) => function1());
  }

  void function1() {
    var trace = _tracing.beginFunction("function1");
    waitForMilliseconds(100);
    function2(42);
    waitForMilliseconds(100);
    trace.end();
  }

  void function2(int x) {
    var trace = _tracing.beginFunction("function2", args: {"x": x});
    waitForMilliseconds(200);
    assert(identity(42) == 42);
    assert(fourtyTwo() == 42);
    assert(addOne(42) == 43);
    doNothing();
    trace.end();
  }

  int identity(int x) {
    return _tracing.trace("identity", () {
      waitForMilliseconds(10);
      return x;
    });
  }

  int addOne(int x) {
    return _tracing.trace("add1", () {
      waitForMilliseconds(10);
      return x + 1;
    }, args: {"x": x});
  }

  int fourtyTwo() {
    return _tracing.trace("fourtyTwo", () {
      waitForMilliseconds(10);
      return 42;
    });
  }

  void doNothing() {
    _tracing.trace("doNothing", () {
      waitForMilliseconds(10);
    });
  }
}

// Uses Mojo's wait() method in order to let time pass.
void waitForMilliseconds(int milliseconds) {
  (new MojoMessagePipe()).endpoints[0].handle.wait(
      MojoHandleSignals.kReadable, milliseconds * 1000);
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  new TestUsingTracingApp.fromHandle(appHandle);
}
