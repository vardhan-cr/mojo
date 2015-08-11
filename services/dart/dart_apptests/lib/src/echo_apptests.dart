// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library echo_apptests;

import 'dart:async';

import 'package:apptest/apptest.dart';
import 'package:mojom/dart/test/echo_service.mojom.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

echoApptests(Application application, String url) {
  group('Echo Service Apptests', () {
    test('String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:mojo_dart_echo_dartzip", echoProxy);

      var v = await echoProxy.ptr.echoString("foo");
      expect(v.value, equals("foo"));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      await echoProxy.close();
    });

    test('Empty String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:mojo_dart_echo_dartzip", echoProxy);

      var v = await echoProxy.ptr.echoString("");
      expect(v.value, equals(""));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      await echoProxy.close();
    });

    test('Null String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:mojo_dart_echo_dartzip", echoProxy);

      var v = await echoProxy.ptr.echoString(null);
      expect(v.value, equals(null));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      await echoProxy.close();
    });

    test('Delayed Success', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:mojo_dart_echo_dartzip", echoProxy);

      var milliseconds = 100;
      var watch = new Stopwatch()..start();
      var v = await echoProxy.ptr.delayedEchoString("foo", milliseconds);
      var elapsed = watch.elapsedMilliseconds;
      expect(v.value, equals("foo"));
      expect(elapsed, greaterThanOrEqualTo(milliseconds));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      await echoProxy.close();
    });

    test('Delayed Close', () {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:mojo_dart_echo_dartzip", echoProxy);

      var milliseconds = 100;
      echoProxy.ptr.delayedEchoString("quit", milliseconds).then((_) {
        throw 'unreachable';
      }, onError: (e) {
        expect(e is ProxyCloseException, isTrue);
      });

      return new Future.delayed(
          new Duration(milliseconds: 10), () => echoProxy.close());
    });
  });
}
