#!mojo mojo:dart_content_handler

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongClientImpl extends PingPongClientStub {
  Completer _completer;
  int _count;

  PingPongClientImpl.unbound(int count, Completer completer) :
      _count = count,
      _completer = completer,
      super.unbound();

  void pong(int pongValue) {
    if (pongValue == _count) {
      _completer.complete(null);
      close();
    }
  }
}

class PingPongServiceImpl extends PingPongServiceStub {
  Application _application;
  PingPongClientProxy _proxy;

  PingPongServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
      : _application = application,
        super(endpoint);

  PingPongServiceImpl.unbound() : super.unbound();

  void setClient(PingPongClientProxy proxy) {
    assert(_proxy == null);
    _proxy = proxy;
  }

  void ping(int pingValue) {
    if (_proxy != null) {
      _proxy.callPong(pingValue + 1);
    }
  }

  pingTargetUrl(String url, int count, Function responseFactory) async {
    var completer = new Completer();
    var pingPongServiceProxy = new PingPongServiceProxy.unbound();
    _application.connectToService(url, pingPongServiceProxy);

    var pingPongClient = new PingPongClientImpl.unbound(count, completer);
    pingPongServiceProxy.callSetClient(pingPongClient);
    pingPongClient.listen();

    for (var i = 0; i < count; i++) {
      pingPongServiceProxy.callPing(i);
    }
    await completer.future;
    pingPongServiceProxy.callQuit();

    return responseFactory(true);
  }

  pingTargetService(PingPongServiceProxy serviceProxy,
                    int count,
                    Function responseFactory) async {
    var completer = new Completer();
    var client = new PingPongClientImpl.unbound(count, completer);
    serviceProxy.callSetClient(client);
    client.listen();

    for (var i = 0; i < count; i++) {
      serviceProxy.callPing(i);
    }
    await completer.future;
    serviceProxy.callQuit();

    return responseFactory(true);
  }

  getPingPongService(PingPongServiceStub stub) {
    stub.delegate = new PingPongServiceImpl.unbound();
    stub.listen();
  }

  void quit() {
    if (_proxy != null) {
      _proxy.close();
    }
    close();
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Function stubFactoryClosure() =>
      (endpoint) => new PingPongServiceImpl(this, endpoint);
}

main(List args) {
  MojoHandle shellHandle = new MojoHandle(args[0]);
  String url = args[1];
  var pingPongApplication = new PingPongApplication.fromHandle(shellHandle);
  pingPongApplication.listen();
}
