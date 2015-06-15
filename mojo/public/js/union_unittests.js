// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

define([
    "gin/test/expect",
    "mojo/public/interfaces/bindings/tests/test_unions.mojom",
    "mojo/public/js/codec",
    "mojo/public/js/validator",
], function(expect,
            unions,
            codec,
            validator) {
  function testConstructors() {
    var u = new unions.PodUnion();
    expect(u.$data).toEqual(null);
    expect(u.$tag).toEqual(unions.PodUnion.Tags.$unknown);

    u.f_uint32 = 32;

    expect(u.f_uint32).toEqual(32);
    expect(u.$tag).toEqual(unions.PodUnion.Tags.f_uint32);

    var u = new unions.PodUnion({f_uint64: 64});
    expect(u.f_uint32).toEqual(64);
    expect(u.$tag).toEqual(unions.PodUnion.Tags.f_uint64);
  }

  testConstructors();
  this.result = "PASS";
});
