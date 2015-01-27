#!mojo mojo:dart_content_handler

// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/pingpong_service.mojom.dart';

class PingPongServiceImpl extends PingPongServiceStub {
  Application _application;
  PingPongClientProxy _proxy;

  PingPongServiceImpl(MojoMessagePipeEndpoint endpoint, Application application)
      : _application = application,
        super(endpoint);

  void setClient(PingPongClientProxy proxy) {
    _proxy = proxy;
  }

  void ping(int pingValue) => _proxy.callPong(pingValue + 1);

  void quit() {
    if (_proxy != null) {
      _proxy.close();
    }
    close();
    if (_application != null) {
      _application.close();
    }
  }
}

class PingPongApplication extends Application {
  PingPongApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Function stubFactoryClosure() =>
      (endpoint) => new PingPongServiceImpl(endpoint, this);
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  var pingPongApplication = new PingPongApplication.fromHandle(appHandle);
  pingPongApplication.listen();
}
