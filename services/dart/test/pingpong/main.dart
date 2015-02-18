// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongClientImpl extends PingPongClient {
  Completer _completer;
  int _count;

  PingPongClientImpl.unbound(this._count, this._completer) : super.unbound() {
    super.delegate = this;
  }

  void pong(int pongValue) {
    if (pongValue == _count) {
      _completer.complete(null);
      close();
    }
  }
}

class PingPongServiceImpl extends PingPongService {
  Application _application;
  PingPongClientProxy _proxy;

  PingPongServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
      : _application = application, super(endpoint) {
    super.delegate = this;
  }

  PingPongServiceImpl.fromStub(PingPongServiceStub stub)
      : super.fromStub(stub) {
    super.delegate = this;
  }

  void setClient(PingPongClientProxy proxy) {
    assert(_proxy == null);
    _proxy = proxy;
  }

  void ping(int pingValue) {
    if (_proxy != null) {
      _proxy.pong(pingValue + 1);
    }
  }

  pingTargetUrl(String url, int count, Function responseFactory) async {
    if (_application == null) {
      return responseFactory(false);
    }
    var completer = new Completer();
    var pingPongService = new PingPongServiceProxy.unbound();
    _application.connectToService(url, pingPongService);

    var pingPongClient = new PingPongClientImpl.unbound(count, completer);
    pingPongService.setClient(pingPongClient.stub);
    pingPongClient.listen();

    for (var i = 0; i < count; i++) {
      pingPongService.ping(i);
    }
    await completer.future;
    pingPongService.quit();

    return responseFactory(true);
  }

  pingTargetService(PingPongServiceProxy pingPongService,
                    int count,
                    Function responseFactory) async {
    var completer = new Completer();
    var client = new PingPongClientImpl.unbound(count, completer);
    pingPongService.setClient(client.stub);
    client.listen();

    for (var i = 0; i < count; i++) {
      pingPongService.ping(i);
    }
    await completer.future;
    pingPongService.quit();

    return responseFactory(true);
  }

  getPingPongService(PingPongServiceStub serviceStub) {
    var pingPongService = new PingPongServiceImpl.fromStub(serviceStub);
    pingPongService.listen();
  }

  void quit() {
    if (_proxy != null) {
      _proxy.close();
    }
    super.close();
    if (_application != null) {
      _application.close();
    }
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void acceptConnection(String requestorUrl, ApplicationConnection connection) {
    connection.provideService(PingPongService.name,
        (endpoint) => new PingPongServiceImpl(this, endpoint));
    connection.listen();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  var pingPongApplication = new PingPongApplication.fromHandle(appHandle);
  pingPongApplication.listen();
}
