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
#include "net/proxy_resolution/proxy_service.h"
#include "net/url_request/url_request_context.h"
#include "vendor/brightray/browser/browser_client.h"
#include "vendor/brightray/browser/net_log.h"
#include "url/third_party/mozilla/url_parse.h"

namespace net {

using content::BrowserThread;
const int kTorPasswordLength = 16;

ProxyConfigServiceTor::ProxyConfigServiceTor(
  const std::string& tor_proxy) {
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
    }
    config_.proxy_rules().ParseFromString(std::string(scheme_ + "://" + host_
      + ":" + port_));
}

ProxyConfigServiceTor::~ProxyConfigServiceTor() {}

void ProxyConfigServiceTor::TorSetProxy(
    scoped_refptr<brightray::URLRequestContextGetter>
      url_request_context_getter,
    const std::string tor_proxy,
    const bool isolated_storage,
    const base::FilePath partition_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (!url_request_context_getter || !isolated_storage)
    return;
  auto proxy_service = url_request_context_getter->GetURLRequestContext()->
    proxy_resolution_service();
  // Notice CreateRequestContextForStoragePartition will only be called once
  // per partition_path so there is no need to cache password per origin
  std::string origin = partition_path.DirName().BaseName().AsUTF8Unsafe();
  std::unique_ptr<net::ProxyConfigServiceTor>
    config(new ProxyConfigServiceTor(tor_proxy));
  config->SetUsername(origin);
  proxy_service->ResetConfigService(std::move(config));
}

ProxyConfigServiceTor::ConfigAvailability
    ProxyConfigServiceTor::GetLatestProxyConfig(ProxyConfig* config) {
  if (scheme_ != kSocksProxy || host_.empty() || port_.empty())
    return CONFIG_UNSET;
  std::string password = GenerateNewPassword();
  std::string url = std::string(scheme_ + "://" + username_ + ":" + password +
    "@" + host_ + ":" + port_);
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
