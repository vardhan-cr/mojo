// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:mojo_bindings' as bindings;
import 'dart:mojo_core' as core;
import 'dart:typed_data';

import 'package:mojo/dart/embedder/test/dart_to_cpp.mojom.dart';

class DartSide {
  static const int BAD_VALUE = 13;
  static const int ELEMENT_BYTES = 1;
  static const int CAPACITY_BYTES = 64;

  core.MojoMessagePipeEndpoint endpoint;
  DartSideInterface interface;
  Uint8List sampleData;
  Uint8List sampleMessage;

  bool receivedPing = false;
  bool receivedEcho = false;

  DartSide(this.endpoint) {
    interface = new DartSideInterface(endpoint);
    sampleData = new Uint8List(CAPACITY_BYTES);
    for (int i = 0; i < sampleData.length; ++i) {
      sampleData[i] = i;
    }
    sampleMessage = new Uint8List(CAPACITY_BYTES);
    for (int i = 0; i < sampleMessage.length; ++i) {
      sampleMessage[i] = 255 - i;
    }
  }

  EchoArgsList createEchoArgsList(List<EchoArgs> list) {
    return list.fold(null, (result, arg) {
      var element = new EchoArgsList();
      element.item = arg;
      element.next = result;
      return element;
    });
  }

  void handlePing() {
    receivedPing = true;
    interface.pingResponse();
  }

  void handleEcho(int numIterations, EchoArgs arg) {
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

      dataPipe1.producer.write(sampleData.buffer.asByteData());
      dataPipe2.producer.write(sampleData.buffer.asByteData());
      messagePipe1.endpoints[1].write(sampleMessage.buffer.asByteData());
      messagePipe2.endpoints[1].write(sampleMessage.buffer.asByteData());

      interface.echoResponse(createEchoArgsList([arg, specialArg]));

      dataPipe1.producer.handle.close();
      dataPipe2.producer.handle.close();
      messagePipe1.endpoints[1].handle.close();
      messagePipe2.endpoints[1].handle.close();
    }
    receivedEcho = true;
    interface.testFinished();
  }

  Future<bool> dartSideInterfaceListen() {
    var completer = new Completer();
    var sub = interface.listen((msg) {
      if (msg is DartSide_Ping_Params) {
        handlePing();
        completer.complete(true);
      } else if (msg is DartSide_Echo_Params) {
        handleEcho(msg.numIterations, msg.arg);
        completer.complete(true);
      } else {
        throw 'Unexpected message: $msg';
      }
    });
    interface.startTest();
    return completer.future;
  }
}

main(List<int> args) {
  assert(args.length == 1);
  int mojoHandle = args[0];
  var rawHandle = new core.RawMojoHandle(mojoHandle);
  var endpoint = new core.MojoMessagePipeEndpoint(rawHandle);
  var dartSide = new DartSide(endpoint);
  dartSide.dartSideInterfaceListen().then((_) {
    assert(dartSide.receivedPing || dartSide.receivedEcho);
  });
}
