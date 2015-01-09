// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;
import 'dart:typed_data';

import 'package:mojo/dart/embedder/test/dart_to_cpp.mojom.dart';

class DartSide extends DartSideInterface {
  static const int BAD_VALUE = 13;
  static const int ELEMENT_BYTES = 1;
  static const int CAPACITY_BYTES = 64;

  Uint8List _sampleData;
  Uint8List _sampleMessage;
  Completer _completer;

  DartSide(core.MojoMessagePipeEndpoint endpoint) : super(endpoint) {
    _sampleData = new Uint8List(CAPACITY_BYTES);
    for (int i = 0; i < _sampleData.length; ++i) {
      _sampleData[i] = i;
    }
    _sampleMessage = new Uint8List(CAPACITY_BYTES);
    for (int i = 0; i < _sampleMessage.length; ++i) {
      _sampleMessage[i] = 255 - i;
    }
    _completer = new Completer();
  }

  EchoArgsList createEchoArgsList(List<EchoArgs> list) {
    return list.fold(null, (result, arg) {
      var element = new EchoArgsList();
      element.item = arg;
      element.next = result;
      return element;
    });
  }

  void ping() {
    callPingResponse();
    _completer.complete(null);
  }

  void echo(int numIterations, EchoArgs arg) {
    if (arg.si64 > 0) {
      arg.si64 = BAD_VALUE;
    }
    if (arg.si32 > 0) {
      arg.si32 = BAD_VALUE;
    }
    if (arg.si16 > 0) {
      arg.si16 = BAD_VALUE;
    }
    if (arg.si8 > 0) {
      arg.si8 = BAD_VALUE;
    }

    for (int i = 0; i < numIterations; ++i) {
      var dataPipe1 = new core.MojoDataPipe(ELEMENT_BYTES, CAPACITY_BYTES);
      var dataPipe2 = new core.MojoDataPipe(ELEMENT_BYTES, CAPACITY_BYTES);
      var messagePipe1 = new core.MojoMessagePipe();
      var messagePipe2 = new core.MojoMessagePipe();

      arg.data_handle = dataPipe1.consumer.handle;
      arg.message_handle = messagePipe1.endpoints[0].handle;

      var specialArg = new EchoArgs();
      specialArg.si64 = -1;
      specialArg.si32 = -1;
      specialArg.si16 = -1;
      specialArg.si8 = -1;
      specialArg.name = 'going';
      specialArg.data_handle = dataPipe2.consumer.handle;
      specialArg.message_handle = messagePipe2.endpoints[0].handle;

      dataPipe1.producer.write(_sampleData.buffer.asByteData());
      dataPipe2.producer.write(_sampleData.buffer.asByteData());
      messagePipe1.endpoints[1].write(_sampleMessage.buffer.asByteData());
      messagePipe2.endpoints[1].write(_sampleMessage.buffer.asByteData());

      callEchoResponse(createEchoArgsList([arg, specialArg]));

      dataPipe1.producer.handle.close();
      dataPipe2.producer.handle.close();
      messagePipe1.endpoints[1].handle.close();
      messagePipe2.endpoints[1].handle.close();
    }
    callTestFinished();
    _completer.complete(null);
  }

  Future<bool> get future => _completer.future;
}


main(List args) {
  assert(args.length == 1);
  int mojoHandle = args[0];
  var rawHandle = new core.MojoHandle(mojoHandle);
  var endpoint = new core.MojoMessagePipeEndpoint(rawHandle);
  var dartSide = new DartSide(endpoint);
  dartSide.listen();
  dartSide.callStartTest();
  dartSide.future.then((_) {
    print('Success');
  });
}
