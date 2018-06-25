// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/net/proxy_resolution/proxy_config_service_tor.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/time/time.h"
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
// Default tor circuit life time is 10 minutes
constexpr base::TimeDelta kTenMins = base::TimeDelta::FromMinutes(10);

ProxyConfigServiceTor::ProxyConfigServiceTor(
  const std::string& tor_proxy, const std::string& username,
  TorProxyMap* tor_proxy_map)
  : tor_proxy_map_(tor_proxy_map) {
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
        // Clear any expired entries in case one is expired.
        ClearExpiredEntries();
        // Look up an entry here.
        auto found = tor_proxy_map->map.find(username);
        std::string password;
        if (found == tor_proxy_map->map.end()) {
          const base::Time now = base::Time::Now();
          password = GenerateNewPassword();
          tor_proxy_map->map.emplace(username, std::make_pair(password, now));
          tor_proxy_map->queue.emplace(now, username);
          // Stop the timer if it was already running -- no need to
          // run it at all for the next ten minutes.
          timer_.Stop();
          // Restart the timer.
          timer_.Start(FROM_HERE, kTenMins, this,
                       &ProxyConfigServiceTor::ClearExpiredEntries);
          std::cerr << "schedule timer at " << base::Time::Now() << std::endl;
        } else {
          password = found->second.first;
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
    tor_proxy_map->map.erase(site_url);
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

void ProxyConfigServiceTor::ClearExpiredEntries() {
  std::cerr << "clear expired entries at " << base::Time::Now() << std::endl;
  const base::Time deadline = base::Time::Now() - kTenMins;
  const std::pair<base::Time, std::string>* entry;
  while (!tor_proxy_map_->queue.empty() &&
         (entry = &tor_proxy_map_->queue.top(), entry->first < deadline)) {
    const std::string& username = entry->second;
    auto found = tor_proxy_map_->map.find(username);
    if (found != tor_proxy_map_->map.end()) {
      // If the expiration date is the same as we had queued, then
      // delete it.  Otherwise, we assume there is a new identity that
      // should last the full ten minutes.
      const base::Time expiration_date = found->second.second;
      if (expiration_date == entry->first) {
        tor_proxy_map_->map.erase(username);
      }
    }
    tor_proxy_map_->queue.pop();
  }
}

}  // namespace net
