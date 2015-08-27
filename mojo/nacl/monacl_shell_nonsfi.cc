// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <fcntl.h>
#include <iostream>

#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/simple_platform_support.h"
#include "mojo/public/platform/nacl/mojo_irt.h"
#include "native_client/src/public/irt_core.h"
#include "native_client/src/public/nonsfi/elf_loader.h"

namespace {

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
  nullptr, // TODO(smklein): Add _MojoGetInitialHandle.
};

const struct nacl_irt_interface kIrtInterfaces[] = {
  { NACL_IRT_MOJO_v0_1, &kIrtMojo, sizeof(kIrtMojo), nullptr }
};

size_t mojo_irt_nonsfi_query(const char* interface_ident,
                             void* table, size_t tablesize) {
  size_t result = nacl_irt_query_list(interface_ident,
                                      table,
                                      tablesize,
                                      kIrtInterfaces,
                                      sizeof(kIrtInterfaces));
  if (result != 0)
    return result;
  return nacl_irt_query_core(interface_ident, table, tablesize);
}

} // namespace

int main(int argc, char** argv, char** environ) {
  nacl_irt_nonsfi_allow_dev_interfaces();
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] <<  " <executable> <args...>\n";
    return 1;
  }
  const char* nexe_filename = argv[1];
  int fd = open(nexe_filename, O_RDONLY);
  if (fd < 0) {
    std::cerr << "Failed to open " << nexe_filename << ": " <<
              strerror(errno) << "\n";
    return 1;
  }
  uintptr_t entry = NaClLoadElfFile(fd);

  mojo::embedder::Init(scoped_ptr<mojo::embedder::PlatformSupport>(
            new mojo::embedder::SimplePlatformSupport()));

  return nacl_irt_nonsfi_entry(argc - 1, argv + 1, environ,
                               reinterpret_cast<nacl_entry_func_t>(entry),
                               mojo_irt_nonsfi_query);
}
