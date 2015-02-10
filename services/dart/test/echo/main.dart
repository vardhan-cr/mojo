// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/echo_service.mojom.dart';

class EchoServiceImpl extends EchoService {
  Application _application;

  EchoServiceImpl(Application application, MojoMessagePipeEndpoint endpoint)
    : _application = application, super(endpoint) {
    super.delegate = this;
  }

  echoString(String value, Function responseFactory) {
    if (value == "quit") {
      close();
      _application.close();
    }
    return new Future.value(responseFactory(value));
  }
}

class EchoApplication extends Application {
  EchoApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void acceptConnection(String requestorUrl, ServiceProvider serviceProvider) {
    serviceProvider.registerFactory(EchoService.name, (endpoint) =>
        new EchoServiceImpl(this, endpoint));
    serviceProvider.listen();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  var echoApplication = new EchoApplication.fromHandle(appHandle);
  echoApplication.listen();
}
