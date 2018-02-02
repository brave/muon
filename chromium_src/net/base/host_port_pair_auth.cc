// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_port_pair.h"

namespace net {

HostPortPair::HostPortPair(const std::string& username,
                           const std::string& password,
                           const std::string& in_host, uint16_t in_port)
    : username_(username), password_(password),
      host_(in_host), port_(in_port) {
}

}  // namespace net
