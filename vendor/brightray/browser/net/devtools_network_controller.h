// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
#define BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_

#include <memory>
#include <unordered_map>

#include "base/macros.h"
#include "content/network/throttling/network_conditions.h"
#include "content/network/throttling/throttling_network_interceptor.h"

using content::ThrottlingNetworkInterceptor;

namespace brightray {

class DevToolsNetworkController {
 public:
  DevToolsNetworkController();
  virtual ~DevToolsNetworkController();

  void SetNetworkState(const std::string& client_id,
                       std::unique_ptr<content::NetworkConditions> conditions);
  ThrottlingNetworkInterceptor* GetInterceptor(
      const std::string& client_id);

 private:
   using InterceptorMap = std::unordered_map<std::string,
         std::unique_ptr<ThrottlingNetworkInterceptor>>;

  std::unique_ptr<ThrottlingNetworkInterceptor> appcache_interceptor_;
  InterceptorMap interceptors_;

  DISALLOW_COPY_AND_ASSIGN(DevToolsNetworkController);
};

}  // namespace brightray

#endif  // BROWSER_DEVTOOLS_NETWORK_CONTROLLER_H_
