// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>

#include "mojo/dart/embedder/io/internet_address.h"

namespace mojo {
namespace dart {

bool InternetAddress::Parse(int type, const char* address, RawAddr* addr) {
  memset(addr, 0, IPV6_RAW_ADDR_LENGTH);
  int result;
  if (type == InternetAddress::TYPE_IPV4) {
    struct sockaddr_in in;
    result = inet_pton(AF_INET, address, &in.sin_addr);
    memmove(addr, &in.sin_addr, IPV4_RAW_ADDR_LENGTH);
  } else {
    CHECK(type == InternetAddress::TYPE_IPV6);
    sockaddr_in6 in6;
    result = inet_pton(AF_INET6, address, &in6.sin6_addr);
    memmove(addr, &in6.sin6_addr, IPV6_RAW_ADDR_LENGTH);
  }
  return result == 1;
}

}  // namespace dart
}  // namespace mojo
