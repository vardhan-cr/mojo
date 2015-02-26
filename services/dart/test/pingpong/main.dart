// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'mojo:application';
import 'mojo:bindings';
import 'mojo:core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongClientImpl implements PingPongClient {
  final PingPongClientStub stub;
  Completer _completer;
  int _count;

  PingPongClientImpl.unbound(this._count, this._completer)
      : stub = new PingPongClientStub.unbound() {
    stub.delegate = this;
  }

  void pong(int pongValue) {
    if (pongValue == _count) {
      _completer.complete(null);
      stub.close();
    }
  }
}

class PingPongServiceImpl implements PingPongService {
  PingPongServiceStub _stub;
  Application _application;
  PingPongClientProxy _pingPongClient;

  PingPongServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
      : _application = application {
    _stub = new PingPongServiceStub.fromEndpoint(endpoint)
            ..delegate = this
            ..listen();
  }

  PingPongServiceImpl.fromStub(this._stub) {
    _stub.delegate = this;
  }

  listen() => _stub.listen();

  void setClient(ProxyBase proxyBase) {
    assert(_pingPongClient== null);
    _pingPongClient = proxyBase;
  }

  void ping(int pingValue) {
    if (_pingPongClient != null) {
      _pingPongClient.ptr.pong(pingValue + 1);
    }
  }

  Future pingTargetUrl(String url, int count, Function responseFactory) async {
    if (_application == null) {
      return responseFactory(false);
    }
    var completer = new Completer();
    var pingPongService= new PingPongServiceProxy.unbound();
    _application.connectToService(url, pingPongService);

    var pingPongClient = new PingPongClientImpl.unbound(count, completer);
    pingPongService.ptr.setClient(pingPongClient.stub);
    pingPongClient.stub.listen();

    for (var i = 0; i < count; i++) {
      pingPongService.ptr.ping(i);
    }
    await completer.future;
    pingPongService.ptr.quit();

    return responseFactory(true);
  }

  Future pingTargetService(ProxyBase proxyBase,
                           int count,
                           Function responseFactory) async {
    var pingPongService = proxyBase;
    var completer = new Completer();
    var client = new PingPongClientImpl.unbound(count, completer);
    pingPongService.ptr.setClient(client.stub);
    client.stub.listen();

    for (var i = 0; i < count; i++) {
      pingPongService.ptr.ping(i);
    }
    await completer.future;
    pingPongService.ptr.quit();

    return responseFactory(true);
  }

  getPingPongService(PingPongServiceStub serviceStub) {
    new PingPongServiceImpl.fromStub(serviceStub)..listen();
  }

  void quit() {
    if (_pingPongClient != null) {
      _pingPongClient.close();
      _pingPongClient = null;
    }
    _stub.close();
    if (_application != null) {
      _application.close();
    }
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void acceptConnection(String requestorUrl, ApplicationConnection connection) {
    connection.provideService(PingPongServiceName,
        (endpoint) => new PingPongServiceImpl(this, endpoint));
    connection.listen();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  new PingPongApplication.fromHandle(appHandle);
}
