// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_
#define ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_

#include <map>
#include <memory>

#include "atom/browser/extensions/atom_extension_system.h"
#include "atom/browser/extensions/atom_extension_system_factory.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "net/base/completion_once_callback.h"

class ChromeExtensionsNetworkDelegate;
class Profile;

namespace extensions {

class EventRouterForwarder;
class InfoMap;

class AtomExtensionsNetworkDelegate : public atom::AtomNetworkDelegate {
 public:
  explicit AtomExtensionsNetworkDelegate(
      Profile* browser_context,
      InfoMap* info_map,
      EventRouterForwarder* event_router);
  ~AtomExtensionsNetworkDelegate() override;

  static void SetAcceptAllCookies(bool accept);

 protected:
  int OnBeforeURLRequest(net::URLRequest* request,
                         net::CompletionOnceCallback callback,
                         GURL* new_url) override;
  int OnBeforeStartTransaction(net::URLRequest* request,
                               net::CompletionOnceCallback callback,
                               net::HttpRequestHeaders* headers) override;
  void OnBeforeRedirect(net::URLRequest* request,
                        const GURL& new_location) override;

 private:
  // NetworkDelegate implementation.
  int OnBeforeURLRequestInternal(
    net::URLRequest* request,
    GURL* new_url);
  int OnBeforeStartTransactionInternal(
    net::URLRequest* request,
    net::HttpRequestHeaders* headers);
  void OnStartTransaction(net::URLRequest* request,
                          const net::HttpRequestHeaders& headers) override;
  int OnHeadersReceivedInternal(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url);
  int OnHeadersReceived(
      net::URLRequest* request,
      net::CompletionOnceCallback callback,
      const net::HttpResponseHeaders* original_response_headers,
      scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
      GURL* allowed_unsafe_redirect_url) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnCompleted(net::URLRequest* request,
                   bool started,
                   int net_error) override;
  void OnURLRequestDestroyed(net::URLRequest* request) override;
  void OnPACScriptError(int line_number, const base::string16& error) override;
  net::NetworkDelegate::AuthRequiredResponse OnAuthRequired(
      net::URLRequest* request,
      const net::AuthChallengeInfo& auth_info,
      AuthCallback callback,
      net::AuthCredentials* credentials) override;
  void RunCallback(base::Callback<int(void)> internal_callback,
                    const uint64_t request_id,
                    int previous_result);

  Profile* profile_;
  std::map<uint64_t, net::CompletionOnceCallback> callbacks_;
  std::unique_ptr<ChromeExtensionsNetworkDelegate> extensions_delegate_;
  extensions::EventRouterForwarder* extension_event_router_forwarder_;

  DISALLOW_COPY_AND_ASSIGN(AtomExtensionsNetworkDelegate);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_ATOM_EXTENSIONS_NETWORK_DELEGATE_H_
