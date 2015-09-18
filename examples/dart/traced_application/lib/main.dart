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
    _tracing = new TracingHelper.fromApplication(
        this, "example_traced_application");
    _tracing.traceInstant("initialized", "traced_application");

    // Now we schedule some random work just so we have something to trace.
    new Timer.periodic(new Duration(seconds: 1), (t) => function1());
  }

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    _tracing.traceInstant("connected", "traced_application");
  }


  Future function1() {
    return _tracing.traceAsync("function1", "traced_application", () async {
      await waitForMilliseconds(100);
      await function2(42);
      await waitForMilliseconds(100);
    });
  }

  Future function2(int x) {
    return _tracing.traceAsync("function2", "traced_application", () async {
      await waitForMilliseconds(200);
      assert(await identity(42) == 42);
      assert(await fourtyTwo() == 42);
      assert(await addOne(42) == 43);
      await doNothing();
    }, args: {"x": x});
  }

  Future identity(int x) {
    return _tracing.traceAsync("identity", "traced_application", () async {
      await waitForMilliseconds(10);
      return x;
    });
  }

  Future addOne(int x) {
    return _tracing.traceAsync("add1", "traced_application", () async {
      await waitForMilliseconds(10);
      return x + 1;
    }, args: {"x": x});
  }

  Future fourtyTwo() {
    return _tracing.traceAsync("fourtyTwo", "traced_application", () async {
      await waitForMilliseconds(10);
      return 42;
    });
  }

  Future doNothing() {
    return _tracing.traceAsync("doNothing", "traced_application", () async {
      await waitForMilliseconds(10);
    });
  }
}

Future waitForMilliseconds(int milliseconds) {
  return new Future.delayed(new Duration(milliseconds: milliseconds), () {});
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  new TestUsingTracingApp.fromHandle(appHandle);
}
