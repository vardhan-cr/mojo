#!mojo mojo:dart_content_handler

// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';

import 'package:services/dart/test/echo_service.mojom.dart';

// TODO(zra): Interface implementations that delegate to another implementation
// will all look the same, more or less. Maybe we should generate them?
class EchoServiceImpl extends EchoServiceInterface {
  EchoServiceInterface _delegate;

  EchoServiceImpl(this._delegate, MojoMessagePipeEndpoint endpoint) :
      super(endpoint);

  echoString(String value) => _delegate.echoString(value);
}

class EchoApplication extends Application implements EchoServiceInterface {
  EchoApplication(MojoMessagePipeEndpoint endpoint) : super(endpoint);

  EchoApplication.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Function interfaceFactoryClosure() {
    return (endpoint) => new EchoServiceImpl(this, endpoint);
  }

  echoString(String value) {
    var response = new EchoServiceEchoStringResponseParams();
    if (value == 'quit') {
      close();
    }
    response.value = value;
    return new Future.value(response);
  }
}

main(List args) {
  MojoHandle shellHandle = new MojoHandle(args[0]);
  String url = args[1];
  var echoApplication = new EchoApplication.fromHandle(shellHandle);
  echoApplication.listen();
}
