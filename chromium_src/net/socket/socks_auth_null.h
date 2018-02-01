// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SOCKS_AUTH_NULL_H_
#define NET_SOCKET_SOCKS_AUTH_NULL_H_

#include "net/socket/socks_client_socket_pool.h"

namespace net {

// Null authentication method.
class NET_EXPORT_PRIVATE SOCKSAuthNull : public SOCKSAuth {
 public:
  SOCKSAuthNull();
  ~SOCKSAuthNull() override;
  SOCKSAuthState* Initialize() const override;
 private:
  class State : public SOCKSAuthState {
   public:
    ~State() override;
    uint8_t method_number() override;
    int Do(int, ClientSocketHandle&, CompletionCallback&) override;
   private:
    friend class SOCKSAuthNull;
    State();
    DISALLOW_COPY_AND_ASSIGN(State);
  };
  DISALLOW_COPY_AND_ASSIGN(SOCKSAuthNull);
};

}  // namespace net

#endif  // NET_SOCKET_SOCKS_AUTH_NULL_H_
