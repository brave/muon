// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/socks_auth_null.h"

namespace net {

// Null authentication method.

SOCKSAuthNull::SOCKSAuthNull() = default;

SOCKSAuthNull::~SOCKSAuthNull() = default;

SOCKSAuthState* SOCKSAuthNull::Initialize() const {
  return new SOCKSAuthNull::State;
}

SOCKSAuthNull::State::State() = default;

SOCKSAuthNull::State::~State() = default;

uint8_t SOCKSAuthNull::State::method_number() {
  return 0x00;
}

int SOCKSAuthNull::State::Do(int rv, ClientSocketHandle& socket,
                             CompletionCallback& callback) {
  return OK;
}

}  // namespace net
