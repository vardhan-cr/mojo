// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"testing"

	"mojo/public/go/bindings"
)

func TestPassMessagePipe(t *testing.T) {
	r, p := bindings.CreateMessagePipeForMojoInterface()
	r1, p1 := r, p
	r1.PassMessagePipe()
	p1.Close()
	if r.PassMessagePipe().IsValid() || p.PassMessagePipe().IsValid() {
		t.Fatal("message pipes should be invalid after PassMessagePipe() or Close()")
	}
}
