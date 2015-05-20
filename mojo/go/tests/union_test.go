// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package tests

import (
	"reflect"
	"testing"

	"mojo/public/go/bindings"
	"mojo/public/interfaces/bindings/tests/test_unions"
)

type unionDecoder func(*bindings.Decoder) (interface{}, error)

type Encodable interface {
	Encode(encoder *bindings.Encoder) error
}

func checkEncoding(t *testing.T, value Encodable, decodeUnion unionDecoder) {
	encoder := bindings.NewEncoder()
	err := value.Encode(encoder)
	if err != nil {
		t.Fatalf("error encoding value %v: %v", value, err)
	}

	bytes, handles, err := encoder.Data()
	if err != nil {
		t.Fatalf("error fetching encoded value %v: %v", value, err)
	}

	decoder := bindings.NewDecoder(bytes, handles)
	decodedValue, err := decodeUnion(decoder)
	if err != nil {
		t.Fatalf("error decoding from bytes %v for tested value %v: %v", bytes, value, err)
	}

	if !reflect.DeepEqual(value, decodedValue) {
		t.Fatalf("unexpected value after decoding: got %v, expected %v", decodedValue, value)
	}
}

func decodePodUnion(decoder *bindings.Decoder) (interface{}, error) {
	return test_unions.DecodePodUnion(decoder)
}

func decodeObjectUnion(decoder *bindings.Decoder) (interface{}, error) {
	return test_unions.DecodeObjectUnion(decoder)
}

func TestBasics(t *testing.T) {
	fint8 := test_unions.PodUnionFInt8{10}
	checkEncoding(t, &fint8, decodePodUnion)

	fint16 := test_unions.PodUnionFInt16{10}
	checkEncoding(t, &fint16, decodePodUnion)

	fdummy := test_unions.ObjectUnionFDummy{test_unions.DummyStruct{10}}
	checkEncoding(t, &fdummy, decodeObjectUnion)

	farray := test_unions.ObjectUnionFArrayInt8{[]int8{1, 2, 3, 4}}
	checkEncoding(t, &farray, decodeObjectUnion)

	fmap := test_unions.ObjectUnionFMapInt8{map[string]int8{"hello": 1, "world": 2}}
	checkEncoding(t, &fmap, decodeObjectUnion)

	fnull := test_unions.ObjectUnionFNullable{}
	checkEncoding(t, &fnull, decodeObjectUnion)
}
