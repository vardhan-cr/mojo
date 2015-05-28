// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"testing"

	"mojo/public/go/bindings"
	"mojo/public/go/system"
	"mojo/public/interfaces/bindings/tests/test_unions"
)

type Encodable interface {
	Encode(encoder *bindings.Encoder) error
}

func TestPodUnion(t *testing.T) {
	tests := []test_unions.PodUnion{
		&test_unions.PodUnionFInt8{8},
		&test_unions.PodUnionFInt16{16},
		&test_unions.PodUnionFUint64{64},
		&test_unions.PodUnionFBool{true},
		&test_unions.PodUnionFEnum{test_unions.AnEnum_Second},
	}

	for _, union := range tests {
		var wrapper, zeroWrapper test_unions.WrapperStruct
		wrapper.PodUnion = union
		check(t, &wrapper, &zeroWrapper)
	}
}

func TestObjectUnion(t *testing.T) {
	tests := []test_unions.ObjectUnion{
		&test_unions.ObjectUnionFDummy{test_unions.DummyStruct{10}},
		&test_unions.ObjectUnionFArrayInt8{[]int8{1, 2, 3, 4}},
		&test_unions.ObjectUnionFMapInt8{map[string]int8{"hello": 1, "world": 2}},
		&test_unions.ObjectUnionFNullable{},
	}

	for _, union := range tests {
		var wrapper, zeroWrapper test_unions.WrapperStruct
		wrapper.ObjectUnion = union
		check(t, &wrapper, &zeroWrapper)
	}
}

func encode(t *testing.T, value Encodable) ([]byte, []system.UntypedHandle) {
	encoder := bindings.NewEncoder()
	err := value.Encode(encoder)
	if err != nil {
		t.Fatalf("error encoding value %v: %v", value, err)
	}

	bytes, handles, err := encoder.Data()
	if err != nil {
		t.Fatalf("error fetching encoded value %v: %v", value, err)
	}

	return bytes, handles
}

func TestNonNullableNullInUnion(t *testing.T) {
	var wrapper test_unions.WrapperStruct
	fdummy := test_unions.ObjectUnionFDummy{test_unions.DummyStruct{10}}
	wrapper.ObjectUnion = &fdummy

	bytes, handles := encode(t, &wrapper)
	bytes[16] = 0

	var decoded test_unions.WrapperStruct
	decoder := bindings.NewDecoder(bytes, handles)

	if err := decoded.Decode(decoder); err == nil {
		t.Fatalf("Null non-nullable should have failed validation.")
	}
}

func TestUnionInStruct(t *testing.T) {
	var ss, out test_unions.SmallStruct
	ss.PodUnion = &test_unions.PodUnionFInt8{10}
	check(t, &ss, &out)

	bytes, _ := encode(t, &ss)
	if int(bytes[8*2]) != 16 {
		t.Fatalf("Union does not start at the correct location in struct.")
	}
}
