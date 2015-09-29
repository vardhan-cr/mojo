// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/interfaces/bindings/tests/rect.mojom.h"
#include "mojo/public/interfaces/bindings/tests/test_structs.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {

template <typename Type>
void SerializeAndDeserialize(Type* val) {
    size_t num_bytes = val->GetSerializedSize();
    char* bytes = new char[num_bytes];
    
    val->Serialize(bytes, num_bytes);

    Type out_val;
    EXPECT_FALSE(val->Equals(out_val));
    out_val.Deserialize(bytes);
    EXPECT_TRUE(val->Equals(out_val));
}

TEST(StructSerializationAPI, BasicStruct) {
    Rect rect;
    rect.x = 123;
    rect.y = 456;
    rect.width = 789;
    rect.height = 999;
    SerializeAndDeserialize(&rect);

    DefaultFieldValues default_values;
    EXPECT_TRUE(default_values.f13.is_null());

    SerializeAndDeserialize(&default_values);
}

}  // namespace test
}  // namespace mojo
