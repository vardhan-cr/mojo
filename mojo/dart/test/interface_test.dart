// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;
import 'dart:typed_data';

import 'package:mojo/dart/testing/expect.dart';

const int kEchoesCount = 100;

class EchoString extends bindings.Struct {
  static const int kStructSize = 16;
  static const bindings.DataHeader kDefaultStructInfo =
      const bindings.DataHeader(kStructSize, 1);
  String a = null;

  EchoString() : super(kStructSize);

  static EchoString deserialize(bindings.Message message) {
    return decode(new bindings.Decoder(message));
  }

  static EchoString decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    EchoString result = new EchoString();
    var mainDataHeader = decoder0.decodeDataHeader();
    if (mainDataHeader.numFields > 0) {
      result.a = decoder0.decodeString(8, false);
    }
    return result;
  }

  void encode(bindings.Encoder encoder) {
    var encoder0 = encoder.getEncoderAtOffset(kDefaultStructInfo);
    encoder0.encodeString(a, 8, false);
  }
}


class EchoStringResponse extends bindings.Struct {
  static const int kStructSize = 16;
  static const bindings.DataHeader kDefaultStructInfo =
      const bindings.DataHeader(kStructSize, 1);
  String a = null;

  EchoStringResponse() : super(kStructSize);

  static EchoStringResponse deserialize(bindings.Message message) {
    return decode(new bindings.Decoder(message));
  }

  static EchoStringResponse decode(bindings.Decoder decoder0) {
    if (decoder0 == null) {
      return null;
    }
    EchoStringResponse result = new EchoStringResponse();
    var mainDataHeader = decoder0.decodeDataHeader();
    if (mainDataHeader.numFields > 0) {
      result.a = decoder0.decodeString(8, false);
    }
    return result;
  }

  void encode(bindings.Encoder encoder) {
    var encoder0 = encoder.getEncoderAtOffset(kDefaultStructInfo);
    encoder0.encodeString(a, 8, false);
  }
}


const int kEchoString_name = 0;
const int kEchoStringResponse_name = 1;

class EchoStub extends bindings.Stub {
  EchoStub(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  Future<EchoStringResponse> echoString(EchoString es) {
    var response = new EchoStringResponse();
    response.a = es.a;
    return new Future.value(response);
  }

  Future<bindings.Message> handleMessage(bindings.ServiceMessage message) {
    switch (message.header.type) {
      case kEchoString_name:
        var es = EchoString.deserialize(message.payload);
        return echoString(es).then((response) {
          if (response != null) {
            return buildResponseWithId(
                response,
                kEchoStringResponse_name,
                message.header.requestId,
                bindings.MessageHeader.kMessageIsResponse);
          }
        });
        break;
      default:
        throw new Exception("Unexpected case");
        break;
    }
    return null;
  }
}


class EchoProxy extends bindings.Proxy {
  EchoProxy(core.MojoMessagePipeEndpoint endpoint) : super(endpoint);

  Future<EchoStringResponse> echoString(String a) {
    // compose message.
    var es = new EchoString();
    es.a = a;
    return sendMessageWithRequestId(
        es,
        kEchoString_name,
        -1,
        bindings.MessageHeader.kMessageExpectsResponse);
  }

  void handleResponse(bindings.ServiceMessage message) {
    switch (message.header.type) {
      case kEchoStringResponse_name:
        var esr = EchoStringResponse.deserialize(message.payload);
        Completer c = completerMap[message.header.requestId];
        completerMap[message.header.requestId] = null;
        c.complete(esr);
        break;
      default:
        throw new Exception("Unexpected case");
        break;
    }
  }
}


void providerIsolate(core.MojoMessagePipeEndpoint endpoint) {
  var provider = new EchoStub(endpoint);
  provider.listen();
}


Future<bool> runTest() async {
  var testCompleter = new Completer();

  var pipe = new core.MojoMessagePipe();
  var proxy = new EchoProxy(pipe.endpoints[0]);
  await Isolate.spawn(providerIsolate, pipe.endpoints[1]);

  int n = kEchoesCount;
  int count = 0;
  for (int i = 0; i < n; i++) {
    proxy.echoString("hello").then((response) {
      Expect.equals("hello", response.a);
      count++;
      if (i == (n - 1)) {
        proxy.close();
        testCompleter.complete(count);
      }
    });
  }

  return testCompleter.future;
}


Future runAwaitTest() async {
  var pipe = new core.MojoMessagePipe();
  var proxy = new EchoProxy(pipe.endpoints[0]);
  await Isolate.spawn(providerIsolate, pipe.endpoints[1]);

  int n = kEchoesCount;
  for (int i = 0; i < n; i++) {
    var response = await proxy.echoString("Hello");
    Expect.equals("Hello", response.a);
  }
  proxy.close();
}


main() async {
  Expect.equals(kEchoesCount, await runTest());
  await runAwaitTest();
}
