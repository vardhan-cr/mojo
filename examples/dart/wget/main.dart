// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Run with, e.g.:
// mojo_shell "mojo:dart_wget http://www.google.com"

import 'dart:async';
import 'dart:mojo_application';
import 'dart:mojo_bindings';
import 'dart:mojo_core';
import 'dart:typed_data';

import 'package:mojo/services/network/public/interfaces/network_service.mojom.dart';
import 'package:mojo/services/network/public/interfaces/url_loader.mojom.dart';

class WGet extends Application {
  NetworkServiceProxy _networkService;
  UrlLoaderProxy _urlLoaderProxy;

  WGet.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args) {
    run(args);
  }

  run(List<String> args) async {
    if (args == null || args.length != 2) {
      throw "Expected URL argument";
    }

    ByteData bodyData = await _getUrl(args[1]);
    print("read ${bodyData.lengthInBytes} bytes");

    _closeProxies();
    close();
  }

  Future<ByteData> _getUrl(String url) async {
    _initProxiesIfNeeded();

    var urlRequest = new UrlRequest()
        ..url = url
        ..autoFollowRedirects = true;

    var urlResponse = await _urlLoaderProxy.start(urlRequest);
    print("url => ${urlResponse.response.url}");
    print("status_line => ${urlResponse.response.statusLine}");
    print("mime_type => ${urlResponse.response.mimeType}");

    return DataPipeDrainer.drainHandle(urlResponse.response.body);
  }

  void _initProxiesIfNeeded() {
    if (_networkService == null) {
      _networkService = new NetworkServiceProxy.unbound();
      connectToService("mojo:network_service", _networkService);
    }
    if (_urlLoaderProxy == null) {
      _urlLoaderProxy = new UrlLoaderProxy.unbound();
      _networkService.createUrlLoader(_urlLoaderProxy);
    }
  }

  void _closeProxies() {
    _urlLoaderProxy.close();
    _networkService.close();
    _urlLoaderProxy = null;
    _networkService = null;
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  String url = args[1];
  var wget = new WGet.fromHandle(appHandle);
  wget.listen();
}
