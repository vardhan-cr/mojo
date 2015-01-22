#!mojo mojo:dart_content_handler

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongServiceImpl extends PingPongServiceInterface {
  PingPongClientClient _client;

  PingPongServiceImpl(MojoMessagePipeEndpoint endpoint) : super(endpoint);

  void setClient(PingPongClientClient client) {
    _client = client;
  }

  void ping(int pingValue) => _client.callPong(pingValue + 1);

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
      (endpoint) => new PingPongServiceImpl(endpoint);
}

main(List args) {
  MojoHandle shellHandle = new MojoHandle(args[0]);
  String url = args[1];
  var pingPongApplication = new PingPongApplication.fromHandle(shellHandle);
  pingPongApplication.listen();
}
