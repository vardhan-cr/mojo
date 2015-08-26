// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

import 'package:service_test_interfaces/test/pingpong_service.mojom.dart';

class PingPongServiceImpl implements PingPongService {
  PingPongServiceStub _stub;
  Application _application;
  PingPongClientProxy _pingPongClient;

  PingPongServiceImpl(this._application, MojoMessagePipeEndpoint endpoint) {
    _stub = new PingPongServiceStub.fromEndpoint(endpoint, this);
  }

  void setClient(ProxyBase proxyBase) {
    assert(_pingPongClient == null);
    _pingPongClient = proxyBase;
  }

  void ping(int pingValue) => _pingPongClient.ptr.pong(pingValue + 1);

  // These methods are unimplemented; they merely throw on invocation.
  Future<PingPongServicePingTargetUrlResponseParams> pingTargetUrl(
          String url, int count, [Function responseFactory]) =>
      throw "Unimplemented";
  Future<PingPongServicePingTargetServiceResponseParams> pingTargetService(
          Object service, int count, [Function responseFactory]) =>
      throw "Unimplemented";
  void getPingPongService(Object service) => throw "Unimplemented";

  void quit() {
    if (_pingPongClient != null) {
      _pingPongClient.close();
      _pingPongClient = null;
    }
    _stub.close();
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  @override
  void acceptConnection(String requestorUrl, String resolvedUrl,
      ApplicationConnection connection) {
    connection.provideService(PingPongServiceName,
        (endpoint) => new PingPongServiceImpl(this, endpoint));
    // Close the application when the first connection goes down.
    connection.onError = closeApplication;
  }

  Future closeApplication() async {
    await close();
    assert(MojoHandle.reportLeakedHandles());
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  new PingPongApplication.fromHandle(appHandle);
}
