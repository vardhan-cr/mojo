// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/nacl/irt_mojo_nonsfi.h"

#include "mojo/public/c/system/functions.h"
#include "mojo/public/platform/nacl/mojo_irt.h"
#include "native_client/src/public/irt_core.h"

namespace {

MojoHandle g_mojo_handle = MOJO_HANDLE_INVALID;

MojoResult _MojoGetInitialHandle(MojoHandle* handle) {
  *handle = g_mojo_handle;
  return MOJO_RESULT_OK;
}

const struct nacl_irt_mojo kIrtMojo = {
    MojoCreateSharedBuffer,
    MojoDuplicateBufferHandle,
    MojoMapBuffer,
    MojoUnmapBuffer,
    MojoCreateDataPipe,
    MojoWriteData,
    MojoBeginWriteData,
    MojoEndWriteData,
    MojoReadData,
    MojoBeginReadData,
    MojoEndReadData,
    MojoGetTimeTicksNow,
    MojoClose,
    MojoWait,
    MojoWaitMany,
    MojoCreateMessagePipe,
    MojoWriteMessage,
    MojoReadMessage,
    _MojoGetInitialHandle,
};

const struct nacl_irt_interface kIrtInterfaces[] = {
    {NACL_IRT_MOJO_v0_1, &kIrtMojo, sizeof(kIrtMojo), nullptr}};

}  // namespace

namespace nacl {

void MojoSetInitialHandle(MojoHandle handle) {
  g_mojo_handle = handle;
}

size_t MojoIrtNonsfiQuery(const char* interface_ident,
                          void* table,
                          size_t tablesize) {
  size_t result = nacl_irt_query_list(interface_ident, table, tablesize,
                                      kIrtInterfaces, sizeof(kIrtInterfaces));
  if (result != 0)
    return result;
  return nacl_irt_query_core(interface_ident, table, tablesize);
}

}  // namespace nacl
