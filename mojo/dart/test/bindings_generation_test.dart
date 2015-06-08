// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:typed_data';

import 'package:_testing/expect.dart';
import 'package:mojo/bindings.dart' as bindings;
import 'package:mojo/core.dart' as core;
import 'package:mojom/sample/sample_interfaces.mojom.dart' as sample;
import 'package:mojom/mojo/test/test_structs.mojom.dart' as structs;
import 'package:mojom/mojo/test/test_unions.mojom.dart' as unions;
import 'package:mojom/mojo/test/rect.mojom.dart' as rect;

class ProviderImpl implements sample.Provider {
  sample.ProviderStub _stub;

  ProviderImpl(core.MojoMessagePipeEndpoint endpoint) {
    _stub = new sample.ProviderStub.fromEndpoint(endpoint, this);
  }

  echoString(String a, Function responseFactory) =>
      new Future.value(responseFactory(a));

  echoStrings(String a, String b, Function responseFactory) =>
      new Future.value(responseFactory(a, b));

  echoMessagePipeHanlde(core.MojoHandle a, Function responseFactory) =>
      new Future.value(responseFactory(a));

  echoEnum(int a, Function responseFactory) =>
      new Future.value(responseFactory(a));
}

void providerIsolate(core.MojoMessagePipeEndpoint endpoint) {
  new ProviderImpl(endpoint);
}

Future<bool> testCallResponse() {
  var pipe = new core.MojoMessagePipe();
  var client = new sample.ProviderProxy.fromEndpoint(pipe.endpoints[0]);
  var c = new Completer();
  Isolate.spawn(providerIsolate, pipe.endpoints[1]).then((_) {
    client.ptr.echoString("hello!").then((echoStringResponse) {
      Expect.equals("hello!", echoStringResponse.a);
    }).then((_) {
      client.ptr.echoStrings("hello", "mojo!").then((echoStringsResponse) {
        Expect.equals("hello", echoStringsResponse.a);
        Expect.equals("mojo!", echoStringsResponse.b);
        client.close();
        c.complete(true);
      });
    });
  });
  return c.future;
}

Future testAwaitCallResponse() async {
  var pipe = new core.MojoMessagePipe();
  var client = new sample.ProviderProxy.fromEndpoint(pipe.endpoints[0]);
  var isolate = await Isolate.spawn(providerIsolate, pipe.endpoints[1]);

  var echoStringResponse = await client.ptr.echoString("hello!");
  Expect.equals("hello!", echoStringResponse.a);

  var echoStringsResponse = await client.ptr.echoStrings("hello", "mojo!");
  Expect.equals("hello", echoStringsResponse.a);
  Expect.equals("mojo!", echoStringsResponse.b);

  client.close();
}

bindings.ServiceMessage messageOfStruct(bindings.Struct s) =>
    s.serializeWithHeader(new bindings.MessageHeader(0));

testSerializeNamedRegion() {
  var r = new rect.Rect()
    ..x = 1
    ..y = 2
    ..width = 3
    ..height = 4;
  var namedRegion = new structs.NamedRegion()
    ..name = "name"
    ..rects = [r];
  var message = messageOfStruct(namedRegion);
  var namedRegion2 = structs.NamedRegion.deserialize(message.payload);
  Expect.equals(namedRegion.name, namedRegion2.name);
}

testSerializeArrayValueTypes() {
  var arrayValues = new structs.ArrayValueTypes()
    ..f0 = [0, 1, -1, 0x7f, -0x10]
    ..f1 = [0, 1, -1, 0x7fff, -0x1000]
    ..f2 = [0, 1, -1, 0x7fffffff, -0x10000000]
    ..f3 = [0, 1, -1, 0x7fffffffffffffff, -0x1000000000000000]
    ..f4 = [0.0, 1.0, -1.0, 4.0e9, -4.0e9]
    ..f5 = [0.0, 1.0, -1.0, 4.0e9, -4.0e9];
  var message = messageOfStruct(arrayValues);
  var arrayValues2 = structs.ArrayValueTypes.deserialize(message.payload);
  Expect.listEquals(arrayValues.f0, arrayValues2.f0);
  Expect.listEquals(arrayValues.f1, arrayValues2.f1);
  Expect.listEquals(arrayValues.f2, arrayValues2.f2);
  Expect.listEquals(arrayValues.f3, arrayValues2.f3);
  Expect.listEquals(arrayValues.f4, arrayValues2.f4);
  Expect.listEquals(arrayValues.f5, arrayValues2.f5);
}

testSerializeStructs() {
  testSerializeNamedRegion();
  testSerializeArrayValueTypes();
}

testSerializePodUnions() {
  var s = new unions.WrapperStruct()
    ..podUnion = new unions.PodUnionWrapper();
  s.podUnion.fFUint32 = 32;

  Expect.equals(unions.PodUnionTag.tagFUint32, s.podUnion.tag);
  Expect.equals(32, s.podUnion.fFUint32);

  var message = messageOfStruct(s);
  var s2 = unions.WrapperStruct.deserialize(message.payload);

  Expect.equals(s.podUnion.fFUint32, s2.podUnion.fFUint32);
}

testSerializeStructInUnion() {
  var s = new unions.WrapperStruct()
    ..objectUnion = new unions.ObjectUnionWrapper();
  s.objectUnion.fFDummy = new unions.DummyStruct()
    ..fInt8 = 8;

  var message = messageOfStruct(s);
  var s2 = unions.WrapperStruct.deserialize(message.payload);

  Expect.equals(s.objectUnion.fFDummy.fInt8, s2.objectUnion.fFDummy.fInt8);
}

testSerializeArrayInUnion() {
  var s = new unions.WrapperStruct()
    ..objectUnion = new unions.ObjectUnionWrapper();
  s.objectUnion.fFArrayInt8 = [1, 2, 3];

  var message = messageOfStruct(s);
  var s2 = unions.WrapperStruct.deserialize(message.payload);

  Expect.listEquals(s.objectUnion.fFArrayInt8, s2.objectUnion.fFArrayInt8);
}

testSerializeMapInUnion() {
  var s = new unions.WrapperStruct()
    ..objectUnion = new unions.ObjectUnionWrapper();
  s.objectUnion.fFMapInt8 = {
    "one": 1,
    "two": 2,
  };

  var message = messageOfStruct(s);
  var s2 = unions.WrapperStruct.deserialize(message.payload);

  Expect.equals(1, s.objectUnion.fFMapInt8["one"]);
  Expect.equals(2, s.objectUnion.fFMapInt8["two"]);
}

testSerializeUnionInArray() {
  var s = new unions.SmallStruct()
    ..podUnionArray = [
      new unions.PodUnionWrapper()
        ..fFUint16 = 16,
      new unions.PodUnionWrapper()
        ..fFUint32 = 32,
    ];

  var message = messageOfStruct(s);

  var s2 = unions.SmallStruct.deserialize(message.payload);

  Expect.equals(16, s2.podUnionArray[0].fFUint16);
  Expect.equals(32, s2.podUnionArray[1].fFUint32);
}

testSerializeUnionInMap() {
  var s = new unions.SmallStruct()
    ..podUnionMap = {
      'one': new unions.PodUnionWrapper()
        ..fFUint16 = 16,
      'two': new unions.PodUnionWrapper()
        ..fFUint32 = 32,
    };

  var message = messageOfStruct(s);

  var s2 = unions.SmallStruct.deserialize(message.payload);

  Expect.equals(16, s2.podUnionMap['one'].fFUint16);
  Expect.equals(32, s2.podUnionMap['two'].fFUint32);
}

testSerializeUnionInUnion() {
  var s = new unions.WrapperStruct()
    ..objectUnion = new unions.ObjectUnionWrapper();
    s.objectUnion.fFPodUnion = new unions.PodUnionWrapper()
        ..fFUint32 = 32;

  var message = messageOfStruct(s);
  var s2 = unions.WrapperStruct.deserialize(message.payload);

  Expect.equals(32, s2.objectUnion.fFPodUnion.fFUint32);
}

testSerializeUnions() {
  testSerializePodUnions();
  testSerializeStructInUnion();
  testSerializeArrayInUnion();
  testSerializeMapInUnion();
  testSerializeUnionInArray();
  testSerializeUnionInMap();
  testSerializeUnionInUnion();
}

void closingProviderIsolate(core.MojoMessagePipeEndpoint endpoint) {
  var provider = new ProviderImpl(endpoint);
  provider._stub.close();
}

Future<bool> runOnClosedTest() {
  var testCompleter = new Completer();
  var pipe = new core.MojoMessagePipe();
  var proxy = new sample.ProviderProxy.fromEndpoint(pipe.endpoints[0]);
  proxy.impl.onError = () => testCompleter.complete(true);
  Isolate.spawn(closingProviderIsolate, pipe.endpoints[1]);
  return testCompleter.future.then((b) {
    Expect.isTrue(b);
  });
}

main() async {
  testSerializeStructs();
  testSerializeUnions();
  await testCallResponse();
  await testAwaitCallResponse();
  await runOnClosedTest();
}
