// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;
import 'dart:typed_data';

import 'package:mojo/dart/testing/expect.dart';
import 'package:mojo/public/interfaces/bindings/tests/sample_interfaces.mojom.dart' as sample;


class ProviderImpl extends sample.ProviderInterface {
  ProviderImpl(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  echoString(String a) {
    var response = new sample.Provider_EchoString_ResponseParams();
    response.a = a;
    return response;
  }

  echoStrings(String a, String b) {
    var response = new sample.Provider_EchoStrings_ResponseParams();
    response.a = a;
    response.b = b;
    return response;
  }

  echoMessagePipeHanlde(core.RawMojoHandle a) {
    var response = new sample.Provider_EchoMessagePipeHandle_ResponseParams();
    response.a = a;
    return response;
  }

  echoEnum(int a) {
    var response = new sample.Provider_EchoEnum_ResponseParams();
    response.a = a;
    return response;
  }
}


void providerIsolate(core.MojoMessagePipeEndpoint endpoint) {
  var provider = new ProviderImpl(endpoint);
  provider.listen();
}


Future<bool> test() {
  var pipe = new core.MojoMessagePipe();
  var client = new sample.ProviderClient(pipe.endpoints[0]);
  var c = new Completer();
  client.open();
  Isolate.spawn(providerIsolate, pipe.endpoints[1]).then((_) {
    client.callEchoString("hello!").then((echoStringResponse) {
      Expect.equals("hello!", echoStringResponse.a);
    }).then((_) {
      client.callEchoStrings("hello", "mojo!").then((echoStringsResponse) {
        Expect.equals("hello", echoStringsResponse.a);
        Expect.equals("mojo!", echoStringsResponse.b);
        client.close();
        c.complete(true);
      });
    });
  });
  return c.future;
}


Future testAwait() async {
  var pipe = new core.MojoMessagePipe();
  var client = new sample.ProviderClient(pipe.endpoints[0]);
  var isolate = await Isolate.spawn(providerIsolate, pipe.endpoints[1]);

  client.open();
  var echoStringResponse = await client.callEchoString("hello!");
  Expect.equals("hello!", echoStringResponse.a);

  var echoStringsResponse = await client.callEchoStrings("hello", "mojo!");
  Expect.equals("hello", echoStringsResponse.a);
  Expect.equals("mojo!", echoStringsResponse.b);

  client.close();
}


main() async {
  await test();
  await testAwait();
}
