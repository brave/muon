// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SOCKS_AUTH_USERNAME_PASSWORD_H_
#define NET_SOCKET_SOCKS_AUTH_USERNAME_PASSWORD_H_

#include "net/base/net_export.h"
#include "net/socket/socks_client_socket_pool.h"

namespace net {

class IOBuffer;

class NET_EXPORT_PRIVATE SOCKSAuthUsernamePassword : public SOCKSAuth {
 public:
  SOCKSAuthUsernamePassword(const std::string&, const std::string&);
  ~SOCKSAuthUsernamePassword() override;
  SOCKSAuthState* Initialize() const override;
 private:
  friend class State;
  std::string username_;
  std::string password_;
  class State : public SOCKSAuthState {
   public:
    ~State() override;
    uint8_t method_number() override;
    int Do(int, ClientSocketHandle&, CompletionCallback&) override;
   private:
    friend class SOCKSAuthUsernamePassword;
    explicit State(const SOCKSAuthUsernamePassword*);
    const SOCKSAuthUsernamePassword* auth_;
    enum {
      STATE_INIT_WRITE = 0,
      STATE_WRITE,
      STATE_WRITE_COMPLETE,
      STATE_INIT_READ,
      STATE_READ,
      STATE_READ_COMPLETE,
      STATE_DONE,
      STATE_BAD,
    } next_state_;
    scoped_refptr<IOBuffer> iobuf_;
    std::string buffer_;
    size_t buffer_left_;
    DISALLOW_COPY_AND_ASSIGN(State);
  };
  DISALLOW_COPY_AND_ASSIGN(SOCKSAuthUsernamePassword);
};

}  // namespace net

#endif  // NET_SOCKET_SOCKS_AUTH_USERNAME_PASSWORD_H_
