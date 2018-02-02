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

HostPortPair::HostPortPair(const std::string& up_host, uint16_t in_port)
    : port_(in_port) {
  std::string::const_iterator begin = up_host.begin();
  std::string::const_iterator end = up_host.end();
  std::string::const_iterator at = std::find(begin, end, '@');
  if (at == end) {
    host_ = up_host;
  } else {
    std::string::const_iterator colon = std::find(begin, at, ':');
    username_ = std::string(begin, colon);
    if (colon != at)
      password_ = std::string(colon + 1, at);
    host_ = std::string(at + 1, end);
  }
}

}  // namespace net
