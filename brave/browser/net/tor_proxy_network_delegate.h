// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_NET_TOR_PROXY_NETWORK_DELEGATE_H_
#define BRAVE_BROWSER_NET_TOR_PROXY_NETWORK_DELEGATE_H_

#include "atom/browser/extensions/atom_extensions_network_delegate.h"
#include "brave/browser/brave_browser_context.h"

namespace extensions {
class EventRouterForwarder;
class InfoMap;
}

namespace brave {

class TorProxyNetworkDelegate :
  public extensions::AtomExtensionsNetworkDelegate {
 public:
  explicit TorProxyNetworkDelegate(
      Profile* browser_context,
      extensions::InfoMap* info_map,
      extensions::EventRouterForwarder* event_router);
  ~TorProxyNetworkDelegate() override;

 private:
  // NetworkDelegate implementation.
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               const net::CompletionCallback& callback,
                               net::HttpRequestHeaders* headers) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;

  void ConfigTorProxyInteral(net::URLRequest* request);

  BraveBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(TorProxyNetworkDelegate);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_TOR_PROXY_NETWORK_DELEGATE_H_
