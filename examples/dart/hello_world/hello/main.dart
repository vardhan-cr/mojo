// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// To run this app:
// mojo_shell mojo:hello

import 'mojo:application';
import 'mojo:bindings';
import 'mojo:core';

class Hello extends Application {
  Hello.fromHandle(MojoHandle handle) : super.fromHandle(handle);

  void initialize(List<String> args, String url) {
    print("$url Hello");
    // We expect to find the world mojo application at the same
    // path as this application.
    connectToApplication(url.replaceAll("hello", "world"));
    close();
  }
}

main(List args) {
  MojoHandle appHandle = new MojoHandle(args[0]);
  var hello = new Hello.fromHandle(appHandle);
  hello.listen();
}
