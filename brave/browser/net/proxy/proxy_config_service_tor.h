// Copyright (c) 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_NET_PROXY_PROXY_CONFIG_SERVICE_TOR_H_
#define BRAVE_BROWSER_NET_PROXY_PROXY_CONFIG_SERVICE_TOR_H_

#include <string>

#include "base/compiler_specific.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service.h"

class GURL;

namespace net {

// Implementation of ProxyConfigService that returns a tor specific result.
class NET_EXPORT ProxyConfigServiceTor : public ProxyConfigService {
 public:
  explicit ProxyConfigServiceTor(const GURL& url);
  ~ProxyConfigServiceTor() override;

  // ProxyConfigService methods:
  void AddObserver(Observer* observer) override {}
  void RemoveObserver(Observer* observer) override {}
  ConfigAvailability GetLatestProxyConfig(ProxyConfig* config) override;

  // Set origin as username
  bool SetUsername(const std::string& username);

 private:
  // Generate a new 128 bit random tag
  std::string GenerateNewPassword();

  ProxyConfig config_;
  GURL url_;
  std::string username_;
};

}  // namespace net

#endif  // BRAVE_BROWSER_NET_PROXY_PROXY_CONFIG_SERVICE_TOR_H_
