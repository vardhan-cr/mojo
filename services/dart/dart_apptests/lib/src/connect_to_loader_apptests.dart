// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

library connectToLoader_apptests;

import 'dart:async';

import 'package:mojo_apptest/apptest.dart';
import 'package:mojo_services/mojo/url_response_disk_cache.mojom.dart';
import 'package:mojo/mojo/url_response.mojom.dart';
import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';

connectToLoaderApptests(Application application, String url) {
  test('Connection', () async {
    var diskCacheProxy = new UrlResponseDiskCacheProxy.unbound();
    application.connectToService("mojo:url_response_disk_cache",
        diskCacheProxy);
    var response = new UrlResponse();
    response.url = 'http://www.example.com';
    await diskCacheProxy.ptr.getFile(response);
    await diskCacheProxy.close();
  });
}
