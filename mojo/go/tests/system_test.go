// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"bytes"
	"testing"
	"unsafe"

	"mojo/go/system/embedder"
	"mojo/public/go/system"
	m "mojo/public/go/system/impl"
)

var core system.Core

const (
	MOJO_HANDLE_SIGNAL_READWRITABLE = m.MojoHandleSignals(m.MOJO_HANDLE_SIGNAL_READABLE |
		m.MOJO_HANDLE_SIGNAL_WRITABLE)
	MOJO_HANDLE_SIGNAL_ALL = m.MojoHandleSignals(m.MOJO_HANDLE_SIGNAL_READABLE |
		m.MOJO_HANDLE_SIGNAL_WRITABLE |
		m.MOJO_HANDLE_SIGNAL_PEER_CLOSED)
)

func init() {
	embedder.InitializeMojoEmbedder()
	core = m.GetCore()
}

func TestGetTimeTicksNow(t *testing.T) {
	x := core.GetTimeTicksNow()
	if x < 10 {
		t.Error("Invalid GetTimeTicksNow return value")
	}
}

func TestMessagePipe(t *testing.T) {
	var h0, h1 m.MojoHandle
	var r m.MojoResult
	var index *uint32
	var state *m.MojoHandleSignalsState
	var states []m.MojoHandleSignalsState

	if r, h0, h1 = core.CreateMessagePipe(nil); r != m.MOJO_RESULT_OK {
		t.Fatalf("CreateMessagePipe failed:%v", r)
	}
	if h0 == m.MOJO_HANDLE_INVALID || h1 == m.MOJO_HANDLE_INVALID {
		t.Fatalf("CreateMessagePipe returned invalid handles h0:%v h1:%v", h0, h1)
	}

	r, state = core.Wait(h0, m.MOJO_HANDLE_SIGNAL_READABLE, 0)
	if r != m.MOJO_RESULT_DEADLINE_EXCEEDED {
		t.Fatalf("h0 should not be readable:%v", r)
	}
	if state == nil {
		t.Fatalf("state should not be nil after CreateMessagePipe")
	}
	if state.SatisfiedSignals != m.MOJO_HANDLE_SIGNAL_WRITABLE {
		t.Fatalf("state should be not be signaled readable after CreateMessagePipe:%v", state.SatisfiedSignals)
	}
	if state.SatisfiableSignals != MOJO_HANDLE_SIGNAL_ALL {
		t.Fatalf("state should allow all signals after CreateMessagePipe:%v", state.SatisfiableSignals)
	}

	r, state = core.Wait(h0, m.MOJO_HANDLE_SIGNAL_WRITABLE, 0)
	if r != m.MOJO_RESULT_OK {
		t.Fatalf("h0 should be writable:%v", r)
	}
	if state == nil {
		t.Fatalf("state should not be nil after core.Wait")
	}
	if state.SatisfiedSignals != m.MOJO_HANDLE_SIGNAL_WRITABLE {
		t.Fatalf("state should be signaled writable after core.Wait:%v", state.SatisfiedSignals)
	}
	if state.SatisfiableSignals != MOJO_HANDLE_SIGNAL_ALL {
		t.Fatalf("state should allow all signals after core.Wait:%v", state.SatisfiableSignals)
	}

	if r, _, _, _, _ = core.ReadMessage(h0, m.MOJO_READ_MESSAGE_FLAG_NONE); r != m.MOJO_RESULT_SHOULD_WAIT {
		t.Fatalf("Read on h0 did not return wait:%v", r)
	}
	kHello := []byte("hello")
	if r = core.WriteMessage(h1, kHello, nil, m.MOJO_WRITE_MESSAGE_FLAG_NONE); r != m.MOJO_RESULT_OK {
		t.Fatalf("Failed WriteMessage on h1:%v", r)
	}

	r, state = core.Wait(h0, m.MOJO_HANDLE_SIGNAL_READABLE, m.MOJO_DEADLINE_INDEFINITE)
	if r != m.MOJO_RESULT_OK {
		t.Fatalf("h0 should be readable after WriteMessage to h1:%v", r)
	}
	if state == nil {
		t.Fatalf("state should not be nil after WriteMessage to h1")
	}
	if state.SatisfiedSignals != MOJO_HANDLE_SIGNAL_READWRITABLE {
		t.Fatalf("h0 should be signaled readable after WriteMessage to h1:%v", state.SatisfiedSignals)
	}
	if state.SatisfiableSignals != MOJO_HANDLE_SIGNAL_ALL {
		t.Fatalf("h0 should be readable/writable after WriteMessage to h1:%v", state.SatisfiableSignals)
	}
	if !state.SatisfiableSignals.IsReadable() || !state.SatisfiableSignals.IsWritable() || !state.SatisfiableSignals.IsClosed() {
		t.Fatalf("Helper functions are misbehaving")
	}

	r, msg, _, _, _ := core.ReadMessage(h0, m.MOJO_READ_MESSAGE_FLAG_NONE)
	if r != m.MOJO_RESULT_OK {
		t.Fatalf("Failed ReadMessage on h0:%v", r)
	}
	if !bytes.Equal(msg, kHello) {
		t.Fatalf("Invalid message expected:%s, got:%s", kHello, msg)
	}

	r, index, states = core.WaitMany([]m.MojoHandle{h0}, []m.MojoHandleSignals{m.MOJO_HANDLE_SIGNAL_READABLE}, 10)
	if r != m.MOJO_RESULT_DEADLINE_EXCEEDED {
		t.Fatalf("h0 should not be readable after reading message:%v", r)
	}
	if index != nil {
		t.Fatalf("should be no index after MOJO_RESULT_DEADLINE_EXCEEDED:%v", index)
	}
	if states == nil || len(states) != 1 {
		t.Fatalf("states should be set after WaitMany:%v", states)
	}
	if states[0].SatisfiedSignals != m.MOJO_HANDLE_SIGNAL_WRITABLE {
		t.Fatalf("h0 should be signaled readable WaitMany:%v", states[0].SatisfiedSignals)
	}
	if states[0].SatisfiableSignals != MOJO_HANDLE_SIGNAL_ALL {
		t.Fatalf("h0 should be readable/writable after WaitMany:%v", states[0].SatisfiableSignals)
	}

	if r = core.Close(h0); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on h0 failed:%v", r)
	}

	r, state = core.Wait(h1, MOJO_HANDLE_SIGNAL_READWRITABLE, 100)
	if r != m.MOJO_RESULT_FAILED_PRECONDITION {
		t.Fatalf("h1 should not be readable/writable after Close(h0):%v", r)
	}
	if state == nil {
		t.Fatalf("state should not be nil after Close(h0)")
	}
	if state.SatisfiedSignals != m.MOJO_HANDLE_SIGNAL_PEER_CLOSED {
		t.Fatalf("state should be signaled closed after Close(h0):%v", state.SatisfiedSignals)
	}
	if state.SatisfiableSignals != m.MOJO_HANDLE_SIGNAL_PEER_CLOSED {
		t.Fatalf("state should only be closable after Close(h0):%v", state.SatisfiableSignals)
	}

	if r = core.Close(h1); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on h1 failed:%v", r)
	}
}

func TestDataPipe(t *testing.T) {
	var hp, hc m.MojoHandle
	var r m.MojoResult

	if r, hp, hc = core.CreateDataPipe(nil); r != m.MOJO_RESULT_OK {
		t.Fatalf("CreateDataPipe failed:%v", r)
	}
	if hp == m.MOJO_HANDLE_INVALID || hc == m.MOJO_HANDLE_INVALID {
		t.Fatalf("CreateDataPipe returned invalid handles hp:%v hc:%v", hp, hc)
	}
	if r, _ = core.Wait(hc, m.MOJO_HANDLE_SIGNAL_READABLE, 0); r != m.MOJO_RESULT_DEADLINE_EXCEEDED {
		t.Fatalf("hc should not be readable:%v", r)
	}
	if r, _ = core.Wait(hp, m.MOJO_HANDLE_SIGNAL_WRITABLE, 0); r != m.MOJO_RESULT_OK {
		t.Fatalf("hp should be writeable:%v", r)
	}
	kHello := []byte("hello")
	r, numBytes := core.WriteData(hp, kHello, m.MOJO_WRITE_DATA_FLAG_NONE)
	if r != m.MOJO_RESULT_OK || numBytes != (uint32)(len(kHello)) {
		t.Fatalf("Failed WriteData on hp:%v numBytes:%d", r, numBytes)
	}
	if r, _ = core.Wait(hc, m.MOJO_HANDLE_SIGNAL_READABLE, 1000); r != m.MOJO_RESULT_OK {
		t.Fatalf("hc should be readable after WriteData on hp:%v", r)
	}
	r, data := core.ReadData(hc, m.MOJO_READ_DATA_FLAG_NONE)
	if r != m.MOJO_RESULT_OK {
		t.Fatalf("Failed ReadData on hc:%v", r)
	}
	if !bytes.Equal(data, kHello) {
		t.Fatalf("Invalid data expected:%s, got:%s", kHello, data)
	}
	if r = core.Close(hp); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on hp failed:%v", r)
	}
	if r, _ = core.Wait(hc, m.MOJO_HANDLE_SIGNAL_READABLE, 100); r != m.MOJO_RESULT_FAILED_PRECONDITION {
		t.Fatalf("hc should not be readable after hp closed:%v", r)
	}
	if r = core.Close(hc); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on hc failed:%v", r)
	}
}

func TestSharedBuffer(t *testing.T) {
	var h0, h1 m.MojoHandle
	var bufPtr unsafe.Pointer
	var r m.MojoResult

	if r, h0 = core.CreateSharedBuffer(nil, 100); r != m.MOJO_RESULT_OK {
		t.Fatalf("CreateSharedBuffer failed:%v", r)
	}
	if h0 == m.MOJO_HANDLE_INVALID {
		t.Fatalf("CreateSharedBuffer returned an invalid handle h0:%v", h0)
	}
	if r, bufPtr = core.MapBuffer(h0, 0, 100, m.MOJO_MAP_BUFFER_FLAG_NONE); r != m.MOJO_RESULT_OK {
		t.Fatalf("MapBuffer failed to map buffer with h0:%v", r)
	}
	bufArrayh0 := (*[100]byte)(bufPtr)
	bufArrayh0[50] = 'x'
	if r, h1 = core.DuplicateBufferHandle(h0, nil); r != m.MOJO_RESULT_OK {
		t.Fatalf("DuplicateBufferHandle of h0 failed:%v", r)
	}
	if h1 == m.MOJO_HANDLE_INVALID {
		t.Fatalf("DuplicateBufferHandle returned an invalid handle h1:%v", h1)
	}
	if r = core.Close(h0); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on h0 failed:%v", r)
	}
	bufArrayh0[51] = 'y'
	if r = core.UnmapBuffer(bufPtr); r != m.MOJO_RESULT_OK {
		t.Fatalf("UnmapBuffer failed:%v", r)
	}
	if r, bufPtr = core.MapBuffer(h1, 50, 50, m.MOJO_MAP_BUFFER_FLAG_NONE); r != m.MOJO_RESULT_OK {
		t.Fatalf("MapBuffer failed to map buffer with h1:%v", r)
	}
	bufArrayh1 := (*[50]byte)(bufPtr)
	if bufArrayh1[0] != 'x' || bufArrayh1[1] != 'y' {
		t.Fatalf("Failed to validate shared buffer. expected:x,y got:%s,%s", bufArrayh1[0], bufArrayh1[1])
	}
	if r = core.UnmapBuffer(bufPtr); r != m.MOJO_RESULT_OK {
		t.Fatalf("UnmapBuffer failed:%v", r)
	}
	if r = core.Close(h1); r != m.MOJO_RESULT_OK {
		t.Fatalf("Close on h1 failed:%v", r)
	}
}
