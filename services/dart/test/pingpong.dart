#!mojo mojo:dart_content_handler

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongClientImpl extends PingPongClientInterface {
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

class PingPongServiceImpl extends PingPongServiceInterface {
  Application _application;
  PingPongClientClient _client;

  PingPongServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
      : _application = application,
        super(endpoint);

  PingPongServiceImpl.unbound() : super.unbound();

  void setClient(PingPongClientClient client) {
    assert(_client == null);
    _client = client;
  }

  void ping(int pingValue) {
    if (_client != null) {
      _client.callPong(pingValue + 1);
    }
  }

  pingTargetUrl(String url, int count, Function responseFactory) async {
    var completer = new Completer();
    var pingPongClientEndpoint = _application.connectToService(
        url, PingPongServiceInterface.name);
    var pingPongServiceClient = new PingPongServiceClient(
        pingPongClientEndpoint);

    var pingPongClient = new PingPongClientImpl.unbound(count, completer);
    pingPongServiceClient.callSetClient(pingPongClient);
    pingPongClient.listen();

    for (var i = 0; i < count; i++) {
      pingPongServiceClient.callPing(i);
    }
    await completer.future;
    pingPongServiceClient.callQuit();

    return responseFactory(true);
  }

  pingTargetService(PingPongServiceClient serviceClient,
                    int count,
                    Function responseFactory) async {
    var completer = new Completer();
    var client = new PingPongClientImpl.unbound(count, completer);
    serviceClient.callSetClient(client);
    client.listen();

    for (var i = 0; i < count; i++) {
      serviceClient.callPing(i);
    }
    await completer.future;
    serviceClient.callQuit();

    return responseFactory(true);
  }

  getPingPongService(PingPongServiceInterface interface) {
    interface.delegate = new PingPongServiceImpl.unbound();
    interface.listen();
  }

  void quit() {
    if (_client != null) {
      _client.close();
    }
    close();
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Function interfaceFactoryClosure() =>
      (endpoint) => new PingPongServiceImpl(this, endpoint);
}

main(List args) {
  MojoHandle shellHandle = new MojoHandle(args[0]);
  String url = args[1];
  var pingPongApplication = new PingPongApplication.fromHandle(shellHandle);
  pingPongApplication.listen();
}
