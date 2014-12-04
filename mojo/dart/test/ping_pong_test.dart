// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';
import 'dart:isolate';
import 'dart:mojo_core' as core;
import 'dart:typed_data';

ByteData byteDataOfString(String s) {
  return new ByteData.view((new Uint8List.fromList(s.codeUnits)).buffer);
}

String stringOfByteData(ByteData bytes) {
  return new String.fromCharCodes(bytes.buffer.asUint8List().toList());
}

void expectStringFromEndpoint(
    String expected, core.MojoMessagePipeEndpoint endpoint) {
  // Query how many bytes are available.
  var result = endpoint.query();
  assert(result != null);
  int size = result.bytesRead;
  assert(size > 0);

  // Read the data.
  ByteData bytes = new ByteData(size);
  result = endpoint.read(bytes);
  assert(result != null);
  assert(size == result.bytesRead);

  // Convert to a string and check.
  String msg = stringOfByteData(bytes);
  assert(expected == msg);
}

void pipeTestIsolate(core.MojoMessagePipeEndpoint endpoint) {
  var handle = new core.MojoHandle(endpoint.handle);
  handle.listen((int signal) {
    if (core.MojoHandleSignals.isReadWrite(signal)) {
      throw 'We should only be reading or writing, not both.';
    } else if (core.MojoHandleSignals.isReadable(signal)) {
      expectStringFromEndpoint("Ping", endpoint);
      handle.enableWriteEvents();
    } else if (core.MojoHandleSignals.isWritable(signal)) {
      endpoint.write(byteDataOfString("Pong"));
      handle.disableWriteEvents();
    } else if (core.MojoHandleSignals.isNone(signal)) {
      handle.close();
    } else {
      throw 'Unexpected signal.';
    }
  });
}

main() {
  var pipe = new core.MojoMessagePipe();
  var endpoint = pipe.endpoints[0];
  var handle = new core.MojoHandle(endpoint.handle);
  Isolate.spawn(pipeTestIsolate, pipe.endpoints[1]).then((_) {
    handle.enableWriteEvents();
    handle.listen((int signal) {
      if (core.MojoHandleSignals.isReadWrite(signal)) {
        throw 'We should only be reading or writing, not both.';
      } else if (core.MojoHandleSignals.isReadable(signal)) {
        expectStringFromEndpoint("Pong", endpoint);
        handle.close();
      } else if (core.MojoHandleSignals.isWritable(signal)) {
        endpoint.write(byteDataOfString("Ping"));
        handle.disableWriteEvents();
      } else if (core.MojoHandleSignals.isNone(signal)) {
        throw 'This end should close first.';
      } else {
        throw 'Unexpected signal.';
      }
    });
  });
}
