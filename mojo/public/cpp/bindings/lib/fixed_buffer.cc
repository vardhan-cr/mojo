// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/public/cpp/bindings/lib/fixed_buffer.h"

#include <stdlib.h>
#include <algorithm>

#include "mojo/public/cpp/bindings/lib/bindings_serialization.h"

namespace mojo {
namespace internal {

//namespace {
//
//const size_t kAlignment = 8;
//
//template <typename T>
//T AlignImpl(T t) {
//  return t + (kAlignment - (t % kAlignment)) % kAlignment;
//}
//
//}  // namespace
//
//size_t Align(size_t size) {
//  return AlignImpl(size);
//}

FixedBuffer::FixedBuffer(size_t size)
    : ptr_(nullptr), cursor_(0), size_(internal::Align(size)) {
  // calloc() required to zero memory and thus avoid info leaks.
  ptr_ = static_cast<char*>(calloc(size_, 1));
}

FixedBuffer::~FixedBuffer() {
  free(ptr_);
}

void* FixedBuffer::Allocate(size_t delta) {
  delta = internal::Align(delta);

  if (delta == 0 || delta > size_ - cursor_) {
//    MOJO_DCHECK(false) << "Not reached";
    return nullptr;
  }

  char* result = ptr_ + cursor_;
  cursor_ += delta;

  return result;
}

void* FixedBuffer::Leak() {
  char* ptr = ptr_;
  ptr_ = nullptr;
  cursor_ = 0;
  size_ = 0;
  return ptr;
}

}  // namespace internal
}  // namespace mojo
