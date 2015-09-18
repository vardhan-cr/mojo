// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/memory.h"

#include <limits>

#include "base/logging.h"
#include "build/build_config.h"

namespace mojo {
namespace system {
namespace internal {

template <size_t alignment>
bool IsAligned(const void* pointer) {
  return reinterpret_cast<uintptr_t>(pointer) % alignment == 0;
}

// MSVS (2010, 2013) sometimes (on the stack) aligns, e.g., |int64_t|s (for
// which |__alignof(int64_t)| is 8) to 4-byte boundaries. http://goo.gl/Y2n56T
#if defined(COMPILER_MSVC) && defined(ARCH_CPU_32_BITS)
template <>
bool IsAligned<8>(const void* pointer) {
  return reinterpret_cast<uintptr_t>(pointer) % 4 == 0;
}
#endif

template <size_t size, size_t alignment>
void CheckUserPointer(const void* pointer) {
  CHECK(pointer && IsAligned<alignment>(pointer));
}

// Explicitly instantiate the sizes we need. Add instantiations as needed.
template void CheckUserPointer<1, 1>(const void*);
template void CheckUserPointer<4, 4>(const void*);
template void CheckUserPointer<8, 4>(const void*);
template void CheckUserPointer<8, 8>(const void*);

template <size_t size, size_t alignment>
void CheckUserPointerWithCount(const void* pointer, size_t count) {
  CHECK_LE(count, std::numeric_limits<size_t>::max() / size);
  CHECK(count == 0 || (pointer && IsAligned<alignment>(pointer)));
}

// Explicitly instantiate the sizes we need. Add instantiations as needed.
template void CheckUserPointerWithCount<1, 1>(const void*, size_t);
template void CheckUserPointerWithCount<4, 4>(const void*, size_t);
template void CheckUserPointerWithCount<8, 4>(const void*, size_t);
template void CheckUserPointerWithCount<8, 8>(const void*, size_t);

template <size_t alignment>
void CheckUserPointerWithSize(const void* pointer, size_t size) {
  // TODO(vtl): If running in kernel mode, do a full verification. For now, just
  // check that it's non-null and aligned. (A faster user mode implementation is
  // also possible if this check is skipped.)
  CHECK(size == 0 || (!!pointer && internal::IsAligned<alignment>(pointer)));
}

// Explicitly instantiate the sizes we need. Add instantiations as needed.
template void CheckUserPointerWithSize<1>(const void*, size_t);
template void CheckUserPointerWithSize<4>(const void*, size_t);
template void CheckUserPointerWithSize<8>(const void*, size_t);

}  // namespace internal
}  // namespace system
}  // namespace mojo
