// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library pingpong_apptests;

import 'dart:async';

import 'package:apptest/apptest.dart';
import 'package:mojom/dart/test/pingpong_service.mojom.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

class _TestingPingPongClient extends PingPongClient {
  final PingPongClientStub stub;
  Completer _completer;

  _TestingPingPongClient.unbound() : stub = new PingPongClientStub.unbound() {
    stub.impl = this;
  }

  waitForPong() async {
    _completer = new Completer();
    return _completer.future;
  }

  void pong(int pongValue) {
    _completer.complete(pongValue);
    _completer = null;
  }
}

pingpongApptests(Application application, String url) {
  group('Ping-Pong Service Apptests', () {
    // Verify that "pingpong.dart" implements the PingPongService interface
    // and sends responses to our client.
    test('Ping Service To Pong Client', () async {
      var pingPongServiceProxy = new PingPongServiceProxy.unbound();
      application.connectToService(
          "mojo:mojo_dart_pingpong_dartzip", pingPongServiceProxy);

      var pingPongClient = new _TestingPingPongClient.unbound();
      pingPongServiceProxy.ptr.setClient(pingPongClient.stub);

      pingPongServiceProxy.ptr.ping(1);
      var pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(2));

      pingPongServiceProxy.ptr.ping(100);
      pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(101));

      await pingPongClient.stub.close();
      await pingPongServiceProxy.close();
    });

    // Verify that "pingpong.dart" can connect to "pingpong_target.dart", act as
    // its client, and return a Future that only resolves after the
    // target.ping() => client.pong() methods have executed 9 times.
    test('Ping Target URL', () async {
      var pingPongServiceProxy = new PingPongServiceProxy.unbound();
      application.connectToService(
          "mojo:mojo_dart_pingpong_dartzip", pingPongServiceProxy);

      var r = await pingPongServiceProxy.ptr.pingTargetUrl(
          "mojo:mojo_dart_pingpong_target_dartzip", 9);
      expect(r.ok, equals(true));

      await pingPongServiceProxy.close();
    });

    // Same as the previous test except that instead of providing the
    // pingpong_target.dart URL, we provide a connection to its PingPongService.
    test('Ping Target Service', () async {
      var pingPongServiceProxy = new PingPongServiceProxy.unbound();
      application.connectToService(
          "mojo:mojo_dart_pingpong_dartzip", pingPongServiceProxy);

      var targetServiceProxy = new PingPongServiceProxy.unbound();
      application.connectToService(
          "mojo:mojo_dart_pingpong_target_dartzip", targetServiceProxy);

      var r = await pingPongServiceProxy.ptr.pingTargetService(
          targetServiceProxy.impl, 9);
      expect(r.ok, equals(true));
      // This side no longer has access to the pipe.
      expect(targetServiceProxy.impl.isOpen, equals(false));
      expect(targetServiceProxy.impl.isBound, equals(false));

      await pingPongServiceProxy.close();
    });

    // Verify that Dart can implement an interface "request" parameter.
    test('Get Target Service', () async {
      var pingPongServiceProxy = new PingPongServiceProxy.unbound();
      application.connectToService(
          "mojo:mojo_dart_pingpong_dartzip", pingPongServiceProxy);

      var targetServiceProxy = new PingPongServiceProxy.unbound();
      pingPongServiceProxy.ptr.getPingPongService(targetServiceProxy);

      var pingPongClient = new _TestingPingPongClient.unbound();
      targetServiceProxy.ptr.setClient(pingPongClient.stub);

      targetServiceProxy.ptr.ping(1);
      var pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(2));

      targetServiceProxy.ptr.ping(100);
      pongValue = await pingPongClient.waitForPong();
      expect(pongValue, equals(101));

      await pingPongClient.stub.close();
      await targetServiceProxy.close();
      await pingPongServiceProxy.close();
    });
  });
}
