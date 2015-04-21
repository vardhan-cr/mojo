// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library io_internet_address_apptests;

import 'dart:async';
import 'dart:mojo.io';

import 'package:apptest/apptest.dart';
import 'package:mojo/public/dart/application.dart';
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';

tests(Application application, String url) {
  group('InternetAddress Apptests', () {
    test('Parse IPv4', () async {
      var localhost = new InternetAddress('127.0.0.1');
      expect(localhost, equals(InternetAddress.LOOPBACK_IP_V4));
    });
    test('Parse IPv6', () async {
      var localhost = new InternetAddress('0:0:0:0:0:0:0:1');
      expect(localhost, equals(InternetAddress.LOOPBACK_IP_V6));
    });
  });
}
