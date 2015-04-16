// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'dart:async';

import 'package:mojo/public/dart/application.dart';
import 'package:mojo/public/dart/bindings.dart';
import 'package:mojo/public/dart/core.dart';
import 'package:mojo/device_info.mojom.dart';

class DeviceInfoApp extends Application {
  final DeviceInfoProxy _deviceInfo = new DeviceInfoProxy.unbound();

  DeviceInfoApp.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  Future initialize(List<String> args, String url) async {
    connectToService("mojo:device_info", _deviceInfo);
    print(await _deviceInfo.ptr.getDeviceType());
    _deviceInfo.close();
    close();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  new DeviceInfoApp.fromHandle(appHandle);
}
