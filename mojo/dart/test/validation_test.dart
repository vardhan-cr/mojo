// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:convert';
import 'dart:isolate';
import 'dart:mojo.builtin' as builtin;
import 'dart:typed_data';

import 'package:mojo/dart/testing/validation_test_input_parser.dart' as parser;
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';
import 'package:mojo/public/interfaces/bindings/tests/validation_test_interfaces.mojom.dart';

class ConformanceTestInterfaceImpl implements ConformanceTestInterface {
  ConformanceTestInterfaceStub _stub;
  Completer _completer;

  ConformanceTestInterfaceImpl(
      this._completer, MojoMessagePipeEndpoint endpoint) {
    _stub = new ConformanceTestInterfaceStub.fromEndpoint(endpoint, this);
  }

  void _complete() => _completer.complete(null);

  method0(double param0) => _complete();
  method1(StructA param0) => _complete();
  method2(StructB param0, StructA param1) => _complete();
  method3(List<bool> param0) => _complete();
  method4(StructC param0, List<int> param1) => _complete();
  method5(StructE param0, MojoDataPipeProducer param1) => _complete();
  method6(List<List<int>> param0) => _complete();
  method7(StructF param0, List<List<int>> param1) => _complete();
  method8(List<List<String>> param0) => _complete();
  method9(List<List<MojoHandle>> param0) => _complete();
  method10(Map<String, int> param0) => _complete();
  method11(StructG param0) => _complete();
  // TODO(yzshen): Why method12 is not here?
  method13(Object param0, int param1, Object param2) => _complete();

  Future close({bool immediate: false}) => _stub.close(immediate: immediate);
}

parser.ValidationParseResult readAndParseTest(String test) {
  List<int> data = builtin.readSync("${test}.data");
  String input = new Utf8Decoder().convert(data).trim();
  return parser.parse(input);
}

String expectedResult(String test) {
  List<int> data = builtin.readSync("${test}.expected");
  return new Utf8Decoder().convert(data).trim();
}

runTest(String name, parser.ValidationParseResult test, String expected) {
  var handles = new List.generate(
      test.numHandles, (_) => new MojoSharedBuffer.create(10).handle);
  var pipe = new MojoMessagePipe();
  var completer = new Completer();
  var conformanceImpl;

  runZoned(() {
    conformanceImpl =
        new ConformanceTestInterfaceImpl(completer, pipe.endpoints[0]);
  }, onError: (e, stackTrace) {
    assert(e is MojoCodecError);
    // TODO(zra): Make the error messages conform?
    // assert(e == expected);
    conformanceImpl.close(immediate: true);
    pipe.endpoints[0].handle.close();
    handles.forEach((h) => h.close());
  });

  var length = (test.data == null) ? 0 : test.data.lengthInBytes;
  var r = pipe.endpoints[1].write(test.data, length, handles);
  assert(r.isOk);

  completer.future.then((_) {
    assert(expected == "PASS");
    conformanceImpl.close();
    pipe.endpoints[0].handle.close();
    handles.forEach((h) => h.close());
  });
}

Iterable<String> getTestFiles(String path, String prefix) => builtin
    .enumerateFiles(path)
    .where((s) => s.startsWith(prefix) && s.endsWith(".data"))
    .map((s) => s.replaceFirst('.data', ''));

main(List args) {
  int handle = args[0];
  String path = args[1];

  // First test the parser.
  parser.parserTests();

  // Then run the conformance tests.
  getTestFiles(path, "$path/conformance_").forEach((test) {
    runTest(test, readAndParseTest(test), expectedResult(test));
  });
  // TODO(zra): Add integration tests when they no longer rely on Client=.
}
