// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHELL_DOMAIN_SOCKET_NET_ERRORS_H__
#define SHELL_DOMAIN_SOCKET_NET_ERRORS_H__

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/files/file.h"

namespace mojo {
namespace shell {
namespace net {

// Error values are negative.
enum Error {
  // No error.
  OK = 0,

#define NET_ERROR(label, value) ERR_##label = value,
#include "shell/domain_socket/net_error_list.h"
#undef NET_ERROR

};

// Returns a textual representation of the error code for logging purposes.
std::string ErrorToString(int error);

// A convenient function to translate file error to net error code.
Error FileErrorToNetError(base::File::Error file_error);

// Map system error code to Error.
Error MapSystemError(int os_error);

}  // namespace net
}  // namespace shell
}  // namespace mojo

#endif  // SHELL_DOMAIN_SOCKET_NET_ERRORS_H__
