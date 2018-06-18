// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/net/proxy_resolution/proxy_config_service_tor.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/values.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/random.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "vendor/brightray/browser/browser_client.h"
#include "vendor/brightray/browser/net_log.h"
#include "url/third_party/mozilla/url_parse.h"

namespace net {

using content::BrowserThread;
const int kTorPasswordLength = 16;

ProxyConfigServiceTor::ProxyConfigServiceTor(
  const std::string& tor_proxy, const std::string& username,
  TorProxyMap* tor_proxy_map) {
    if (tor_proxy.length()) {
      url::Parsed url;
      url::ParseStandardURL(
        tor_proxy.c_str(),
        std::min(tor_proxy.size(),
                 static_cast<size_t>(std::numeric_limits<int>::max())),
        &url);
      if (url.scheme.is_valid()) {
        scheme_ =
          std::string(tor_proxy.begin() + url.scheme.begin,
                      tor_proxy.begin() + url.scheme.begin + url.scheme.len);
      }
      if (url.host.is_valid()) {
        host_ =
          std::string(tor_proxy.begin() + url.host.begin,
                      tor_proxy.begin() + url.host.begin + url.host.len);
      }
      if (url.port.is_valid()) {
        port_ =
          std::string(tor_proxy.begin() + url.port.begin,
                      tor_proxy.begin() + url.port.begin + url.port.len);
      }
      std::string proxy_url;
      if (tor_proxy_map || username.empty()) {
        auto found = tor_proxy_map->find(username);
        std::string password;
        if (found == tor_proxy_map->end()) {
          password = GenerateNewPassword();
          tor_proxy_map->insert(std::pair<std::string, std::string>(username,
                                                                    password));
        } else {
          password = found->second;
        }
        proxy_url = std::string(scheme_ + "://" + username + ":" + password +
                                "@" + host_ + ":" + port_);
      } else {
        proxy_url = std::string(scheme_ + "://" + host_ + ":" + port_);
      }
      config_.proxy_rules().ParseFromString(proxy_url);
    }
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

void ProxyConfigServiceTor::TorSetProxy(
    net::ProxyResolutionService* service,
    const std::string& tor_proxy,
    const std::string& site_url,
    TorProxyMap* tor_proxy_map,
    bool new_password) {
  if (!service)
    return;
  if (new_password && tor_proxy_map)
    tor_proxy_map->erase(site_url);
  std::unique_ptr<net::ProxyConfigServiceTor>
    config(new ProxyConfigServiceTor(tor_proxy, site_url, tor_proxy_map));
  service->ResetConfigService(std::move(config));
}

ProxyConfigServiceTor::ConfigAvailability
    ProxyConfigServiceTor::GetLatestProxyConfig(
      ProxyConfigWithAnnotation* config) {
  if (scheme_ != kSocksProxy || host_.empty() || port_.empty())
    return CONFIG_UNSET;
  *config = net:: ProxyConfigWithAnnotation(config_, NO_TRAFFIC_ANNOTATION_YET);
  return CONFIG_VALID;
}

std::string ProxyConfigServiceTor::GenerateNewPassword() {
  std::vector<uint8_t> password(kTorPasswordLength);
  crypto::RandBytes(password.data(), password.size());
  return base::HexEncode(password.data(), password.size());
}

}  // namespace net
