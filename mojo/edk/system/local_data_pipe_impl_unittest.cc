// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/local_data_pipe_impl.h"

#include <string.h>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "mojo/edk/system/data_pipe.h"
#include "mojo/edk/system/waiter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace system {
namespace {

const uint32_t kSizeOfOptions =
    static_cast<uint32_t>(sizeof(MojoCreateDataPipeOptions));

// Validate options.
// TODO(vtl): Maybe move this to |DataPipeImplTest| (and also test transferring
// these data pipes)?
TEST(LocalDataPipeImplTest, Creation) {
  // Create using default options.
  {
    // Get default options.
    MojoCreateDataPipeOptions default_options = {0};
    EXPECT_EQ(MOJO_RESULT_OK, DataPipe::ValidateCreateOptions(
                                  NullUserPointer(), &default_options));
    scoped_refptr<DataPipe> dp(DataPipe::CreateLocal(default_options));
    dp->ProducerClose();
    dp->ConsumerClose();
  }

  // Create using non-default options.
  {
    const MojoCreateDataPipeOptions options = {
        kSizeOfOptions,                           // |struct_size|.
        MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
        1,                                        // |element_num_bytes|.
        1000                                      // |capacity_num_bytes|.
    };
    MojoCreateDataPipeOptions validated_options = {0};
    EXPECT_EQ(MOJO_RESULT_OK,
              DataPipe::ValidateCreateOptions(MakeUserPointer(&options),
                                              &validated_options));
    scoped_refptr<DataPipe> dp(DataPipe::CreateLocal(validated_options));
    dp->ProducerClose();
    dp->ConsumerClose();
  }
  {
    const MojoCreateDataPipeOptions options = {
        kSizeOfOptions,                           // |struct_size|.
        MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
        4,                                        // |element_num_bytes|.
        4000                                      // |capacity_num_bytes|.
    };
    MojoCreateDataPipeOptions validated_options = {0};
    EXPECT_EQ(MOJO_RESULT_OK,
              DataPipe::ValidateCreateOptions(MakeUserPointer(&options),
                                              &validated_options));
    scoped_refptr<DataPipe> dp(DataPipe::CreateLocal(validated_options));
    dp->ProducerClose();
    dp->ConsumerClose();
  }
  // Default capacity.
  {
    const MojoCreateDataPipeOptions options = {
        kSizeOfOptions,                           // |struct_size|.
        MOJO_CREATE_DATA_PIPE_OPTIONS_FLAG_NONE,  // |flags|.
        100,                                      // |element_num_bytes|.
        0                                         // |capacity_num_bytes|.
    };
    MojoCreateDataPipeOptions validated_options = {0};
    EXPECT_EQ(MOJO_RESULT_OK,
              DataPipe::ValidateCreateOptions(MakeUserPointer(&options),
                                              &validated_options));
    scoped_refptr<DataPipe> dp(DataPipe::CreateLocal(validated_options));
    dp->ProducerClose();
    dp->ConsumerClose();
  }
}

}  // namespace
}  // namespace system
}  // namespace mojo
