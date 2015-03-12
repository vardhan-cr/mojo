// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo.application';
import 'dart:mojo.bindings';
import 'dart:mojo.core';

import 'package:services/dart/test/echo_service.mojom.dart';

class EchoServiceImpl implements EchoService {
  EchoServiceStub _stub;
  Application _application;

  EchoServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _stub = new EchoServiceStub.fromEndpoint(endpoint, this);
  }

  echoString(String value, [Function responseFactory]) {
    if (value == "quit") {
      _stub.close();
    }
    return new Future.value(responseFactory(value));
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
