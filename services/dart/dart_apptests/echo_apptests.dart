// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library echo_apptests;

import 'dart:async';
import 'dart:mojo.application';
import 'dart:mojo.bindings';
import 'dart:mojo.core';

import 'package:apptest/apptest.dart';
import 'package:services/dart/test/echo_service.mojom.dart';

echoApptests(Application application, String url) {
  group('Echo Service Apptests', () {
    test('String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:dart_echo", echoProxy);

      var v = await echoProxy.ptr.echoString("foo");
      expect(v.value, equals("foo"));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      echoProxy.close();
    });

    test('Empty String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:dart_echo", echoProxy);

      var v = await echoProxy.ptr.echoString("");
      expect(v.value, equals(""));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      echoProxy.close();
    });

    test('Null String', () async {
      var echoProxy = new EchoServiceProxy.unbound();
      application.connectToService("mojo:dart_echo", echoProxy);

      var v = await echoProxy.ptr.echoString(null);
      expect(v.value, equals(null));

      var q = await echoProxy.ptr.echoString("quit");
      expect(q.value, equals("quit"));

      echoProxy.close();
    });
  });
}
