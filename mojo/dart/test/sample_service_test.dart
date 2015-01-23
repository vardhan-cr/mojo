// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;

import 'package:mojo/public/interfaces/bindings/tests/sample_service.mojom.dart' as sample;

class ExpectPortStubImpl implements sample.PortStub {
  String _expected;
  ExpectPortStubImpl([this._expected = ""]);

  void postMessage(String messageText, sample.PortProxy port) {
    assert(messageText == _expected);
    port.close();
  }
}

class ServiceImpl extends sample.ServiceStub {
  ServiceImpl(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  void frobinate(sample.Foo foo, int baz, sample.PortProxy portProxy) {
    var portStub = new sample.PortStub.unbound();
    portStub.delegate = new ExpectPortStubImpl();
    portProxy.callPostMessage("frobinated", portStub);
    portStub.listen();
    callDidFrobinate(42);
    portProxy.close();
  }

  void getPort(sample.PortStub portStub) {
    portStub.delegate = new ExpectPortStubImpl("port");
    portStub.listen();
  }
}

class ServiceClientImpl extends sample.ServiceClientStub
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
    var portProxy = new sample.PortProxy.unbound();
    callGetPort(portProxy);
    portProxy.close();

    var portStub = new sample.PortStub.unbound();
    portStub.delegate = new ExpectPortStubImpl("frobinated");
    callFrobinate(new sample.Foo(), sample.BazOptions_EXTRA, portStub);
    portStub.listen();
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
