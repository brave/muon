// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <iostream>
#include <string>

#include "net/base/url_auth_util.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon_ip.h"

namespace net {

// Copypasta of ParseHostAndPort that extracts the username and
// password instead of rejecting them.
bool ParseAuthHostAndPort(std::string::const_iterator host_and_port_begin,
                          std::string::const_iterator host_and_port_end,
                          std::string *username,
                          std::string *password,
                          std::string* host,
                          int* port) {
  if (host_and_port_begin >= host_and_port_end)
    return false;

  // When using url, we use char*.
  const char* auth_begin = &(*host_and_port_begin);
  int auth_len = host_and_port_end - host_and_port_begin;

  url::Component auth_component(0, auth_len);
  url::Component username_component;
  url::Component password_component;
  url::Component hostname_component;
  url::Component port_component;

  url::ParseAuthority(auth_begin, auth_component, &username_component,
      &password_component, &hostname_component, &port_component);

  if (!hostname_component.is_nonempty())
    return false;  // Failed parsing.

  int parsed_port_number = -1;
  if (port_component.is_nonempty()) {
    parsed_port_number = url::ParsePort(auth_begin, port_component);

    // If parsing failed, port_number will be either PORT_INVALID or
    // PORT_UNSPECIFIED, both of which are negative.
    if (parsed_port_number < 0)
      return false;  // Failed parsing the port number.
  }

  if (port_component.len == 0)
    return false;  // Reject inputs like "foo:"

  unsigned char tmp_ipv6_addr[16];

  // If the hostname starts with a bracket, it is either an IPv6 literal or
  // invalid. If it is an IPv6 literal then strip the brackets.
  if (hostname_component.len > 0 &&
      auth_begin[hostname_component.begin] == '[') {
    if (auth_begin[hostname_component.end() - 1] == ']' &&
        url::IPv6AddressToNumber(
            auth_begin, hostname_component, tmp_ipv6_addr)) {
      // Strip the brackets.
      hostname_component.begin++;
      hostname_component.len -= 2;
    } else {
      return false;
    }
  }

  // Pass results back to caller.
  username->assign(auth_begin + username_component.begin,
                   username_component.len);
  password->assign(auth_begin + password_component.begin,
                   password_component.len);
  host->assign(auth_begin + hostname_component.begin, hostname_component.len);
  *port = parsed_port_number;

  std::cerr << "username(" << username->size() << "): " << *username << "\n";
  std::cerr << "password(" << password->size() << "): " << *password << "\n";
  return true;  // Success.
}

}
