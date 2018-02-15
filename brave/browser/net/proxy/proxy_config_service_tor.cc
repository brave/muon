// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/net/proxy/proxy_config_service_tor.h"

#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "crypto/random.h"
#include "url/gurl.h"

namespace net {

const int kTorPasswordLength = 16;

ProxyConfigServiceTor::ProxyConfigServiceTor(const GURL& url)
  : url_(url) {
    config_.proxy_rules().ParseFromString(url_.spec());
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

ProxyConfigServiceTor::ConfigAvailability
    ProxyConfigServiceTor::GetLatestProxyConfig(ProxyConfig* config) {
  if (!url_.is_valid())
    return CONFIG_UNSET;
  std::string password = GenerateNewPassword();
  std::string url = url_.spec();
  base::ReplaceFirstSubstringAfterOffset(
    &url, 0, "//", "//" + username_ + ":" + password + "@");
  config_.proxy_rules().ParseFromString(url);
  *config = config_;
  return CONFIG_VALID;
}

bool ProxyConfigServiceTor::SetUsername(const std::string& username) {
  if (username.empty())
    return false;
  username_ = username;
  return true;
}


std::string ProxyConfigServiceTor::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

}  // namespace net
