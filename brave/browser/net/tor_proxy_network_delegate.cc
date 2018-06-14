// Copyright 2018 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/net/tor_proxy_network_delegate.h"

#include "brave/browser/net/proxy_resolution/proxy_config_service_tor.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/site_instance.h"
#include "extensions/common/constants.h"
#include "net/base/url_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"

using content::BrowserThread;

namespace brave {

TorProxyNetworkDelegate::TorProxyNetworkDelegate(
      Profile* profile,
      extensions::InfoMap* info_map,
      extensions::EventRouterForwarder* event_router) :
      extensions::AtomExtensionsNetworkDelegate(profile, info_map,
                                                event_router),
      browser_context_(static_cast<BraveBrowserContext*>(profile)) {}

TorProxyNetworkDelegate::~TorProxyNetworkDelegate() {}

int TorProxyNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  ConfigTorProxyInteral(request);
  return extensions::AtomExtensionsNetworkDelegate::OnBeforeURLRequest(request,
                                                                       callback,
                                                                       new_url);
}

int TorProxyNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {
  ConfigTorProxyInteral(request);
  return extensions::AtomExtensionsNetworkDelegate::
    OnBeforeStartTransaction(request,
                             callback,
                             headers);
}

void TorProxyNetworkDelegate::OnBeforeRedirect(
    net::URLRequest* request,
    const GURL& new_location) {
  ConfigTorProxyInteral(request);
  extensions::AtomExtensionsNetworkDelegate::OnBeforeRedirect(request,
                                                              new_location);
}

void TorProxyNetworkDelegate::ConfigTorProxyInteral(net::URLRequest* request) {
  if (!request)
    return;
  auto proxy_service = request->context()->proxy_resolution_service();
  GURL url(content::SiteInstance::GetSiteForURL(browser_context_,
                                                     request->url()));
  if (!url.SchemeIs(extensions::kExtensionScheme) && !net::IsLocalhost(url))
    BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(&net::ProxyConfigServiceTor::TorSetProxy,
                                       proxy_service,
                                       browser_context_->tor_proxy(),
                                       url.host(),
                                       browser_context_->tor_proxy_map(),
                                       false));
}

}  // namespace brave
