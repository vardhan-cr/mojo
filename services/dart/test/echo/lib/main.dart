// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:mojom/dart/test/echo_service.mojom.dart';

class EchoServiceImpl implements EchoService {
  EchoServiceStub _stub;
  Application _application;

  EchoServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _stub = new EchoServiceStub.fromEndpoint(endpoint, this);
  }

  Future echoString(String value, [Function responseFactory]) {
    if (value == "quit") {
      _stub.close();
    }
    return new Future.value(responseFactory(value));
  }

  Future delayedEchoString(String value, int millis,
      [Function responseFactory]) {
    if (value == "quit") {
      _stub.close();
    }
    return new Future.delayed(
        new Duration(milliseconds: millis), () => responseFactory(value));
  }
}

class EchoApplication extends Application {
  EchoApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(
        EchoServiceName, (endpoint) => new EchoServiceImpl(this, endpoint));
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  new EchoApplication.fromHandle(appHandle)
    ..onError = (() {
      assert(MojoHandle.reportLeakedHandles());
    });
}
