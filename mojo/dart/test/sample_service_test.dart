// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;

import 'package:mojo/public/interfaces/bindings/tests/sample_service.mojom.dart' as sample;

class ExpectPortInterfaceImpl implements sample.PortInterface {
  String _expected;
  ExpectPortInterfaceImpl([this._expected = ""]);

  void postMessage(String messageText, sample.PortClient port) {
    assert(messageText == _expected);
    port.close();
  }
}

class ServiceImpl extends sample.ServiceInterface {
  ServiceImpl(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  void frobinate(sample.Foo foo, int baz, sample.PortClient portClient) {
    var portInterface = new sample.PortInterface.unbound();
    portInterface.delegate = new ExpectPortInterfaceImpl();
    portClient.callPostMessage("frobinated", portInterface);
    portInterface.listen();
    callDidFrobinate(42);
    portClient.close();
  }

  void getPort(sample.PortInterface portInterface) {
    portInterface.delegate = new ExpectPortInterfaceImpl("port");
    portInterface.listen();
  }
}

class ServiceClientImpl extends sample.ServiceClientInterface
                        with sample.ServiceCalls {
  Completer completer;

  ServiceClientImpl(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  void didFrobinate(int result) {
    assert(result == 42);
    completer.complete(null);
  }

  Future run() {
    completer = new Completer();

    listen();
    var portClient = new sample.PortClient.unbound();
    callGetPort(portClient);
    portClient.close();

    var portInterface = new sample.PortInterface.unbound();
    portInterface.delegate = new ExpectPortInterfaceImpl("frobinated");
    callFrobinate(new sample.Foo(), sample.BazOptions_EXTRA, portInterface);
    portInterface.listen();
    return completer.future;
  }
}

void serviceIsolate(core.MojoMessagePipeEndpoint endpoint) {
  var service = new ServiceImpl(endpoint);
  service.listen();
}

main() async {
  var pipe = new core.MojoMessagePipe();
  var isolate = await Isolate.spawn(serviceIsolate, pipe.endpoints[0]);

  var serviceClient = new ServiceClientImpl(pipe.endpoints[1]);
  await serviceClient.run();

  serviceClient.close();
}
