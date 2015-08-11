#!mojo mojo:dart_content_handler
// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Run with, e.g.:
// mojo_shell "mojo:dart_wget http://www.google.com"

import 'dart:async';
import 'dart:typed_data';

import 'package:mojo/application.dart';
import 'package:mojo/bindings.dart';
import 'package:mojo/core.dart';
import 'package:mojo/mojo/url_request.mojom.dart';
import 'package:mojo_services/mojo/network_service.mojom.dart';
import 'package:mojo_services/mojo/url_loader.mojom.dart';

class WGet extends Application {
  NetworkServiceProxy _networkService;
  UrlLoaderProxy _urlLoader;

  WGet.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    run(args);
  }

  run(List<String> args) async {
    if (args == null || args.length != 2) {
      throw "Expected URL argument";
    }

    ByteData bodyData = await _getUrl(args[1]);
    print(">>> Body <<<");
    print(new String.fromCharCodes(bodyData.buffer.asUint8List()));
    print(">>> EOF <<<");

    _closeProxies();
    await close();
    assert(MojoHandle.reportLeakedHandles());
  }

  Future<ByteData> _getUrl(String url) async {
    _initProxiesIfNeeded();

    var urlRequest = new UrlRequest()
      ..url = url
      ..autoFollowRedirects = true;

    var urlResponse = await _urlLoader.ptr.start(urlRequest);
    print(">>> Headers <<<");
    print(urlResponse.response.headers.join('\n'));

    return DataPipeDrainer.drainHandle(urlResponse.response.body);
  }

  void _initProxiesIfNeeded() {
    if (_networkService == null) {
      _networkService = new NetworkServiceProxy.unbound();
      connectToService("mojo:network_service", _networkService);
    }
    if (_urlLoader == null) {
      _urlLoader = new UrlLoaderProxy.unbound();
      _networkService.ptr.createUrlLoader(_urlLoader);
    }
  }

  void _closeProxies() {
    _urlLoader.close();
    _urlLoader = null;
    _networkService.close();
    _networkService = null;
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  new WGet.fromHandle(appHandle);
}
