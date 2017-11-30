// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/atom_extensions_network_delegate.h"

#include "base/stl_util.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
#include "chrome/browser/net/chrome_extensions_network_delegate.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "extensions/browser/info_map.h"
#include "net/url_request/url_request.h"

namespace extensions {

namespace {
bool g_accept_all_cookies = true;
}

AtomExtensionsNetworkDelegate::AtomExtensionsNetworkDelegate(
      Profile* profile,
      InfoMap* info_map,
      EventRouterForwarder* event_router) :
      profile_(profile),
      extension_event_router_forwarder_(event_router) {
  extensions_delegate_.reset(
      ChromeExtensionsNetworkDelegate::Create(event_router));
  extensions_delegate_->set_profile(profile);
  extensions_delegate_->set_extension_info_map(info_map);
}

AtomExtensionsNetworkDelegate::~AtomExtensionsNetworkDelegate() {}

void AtomExtensionsNetworkDelegate::SetAcceptAllCookies(bool accept) {
  g_accept_all_cookies = accept;
}

void AtomExtensionsNetworkDelegate::RunCallback(
                                  base::Callback<int(void)> internal_callback,
                                  const uint64_t request_id,
                                  int previous_result) {
  if (!ContainsKey(callbacks_, request_id))
    return;

  if (previous_result == net::OK) {
    int result = internal_callback.Run();

    if (result != net::ERR_IO_PENDING) {
      // nothing ran the original callback
      callbacks_[request_id].Run(net::OK);
    }
  } else {
    // nothing ran the original callback
    callbacks_[request_id].Run(previous_result);
  }

  // make sure this don't run again
  callbacks_.erase(request_id);
}

int AtomExtensionsNetworkDelegate::OnBeforeURLRequestInternal(
    net::URLRequest* request,
    GURL* new_url) {
  return atom::AtomNetworkDelegate::OnBeforeURLRequest(
      request, callbacks_[request->identifier()], new_url);
}

int AtomExtensionsNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  extensions_delegate_->ForwardStartRequestStatus(request);

  callbacks_[request->identifier()] = callback;

  base::Callback<int(void)> internal_callback = base::Bind(
          &AtomExtensionsNetworkDelegate::OnBeforeURLRequestInternal,
          base::Unretained(this),
          request,
          new_url);

  auto wrapped_cb = base::Bind(&AtomExtensionsNetworkDelegate::RunCallback,
                                base::Unretained(this),
                                internal_callback,
                                request->identifier());

  int result = extensions_delegate_->OnBeforeURLRequest(
      request, wrapped_cb, new_url);

  if (result == net::OK)
    return internal_callback.Run();

  return result;
}

int AtomExtensionsNetworkDelegate::OnBeforeStartTransactionInternal(
    net::URLRequest* request,
    net::HttpRequestHeaders* headers) {
  return atom::AtomNetworkDelegate::OnBeforeStartTransaction(
    request,
    callbacks_[request->identifier()],
    headers);
}

int AtomExtensionsNetworkDelegate::OnBeforeStartTransaction(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    net::HttpRequestHeaders* headers) {

  callbacks_[request->identifier()] = callback;

  base::Callback<int(void)> internal_callback = base::Bind(
          &AtomExtensionsNetworkDelegate::OnBeforeStartTransactionInternal,
          base::Unretained(this),
          request,
          headers);

  auto wrapped_cb = base::Bind(&AtomExtensionsNetworkDelegate::RunCallback,
                                base::Unretained(this),
                                internal_callback,
                                request->identifier());

  int result = extensions_delegate_->OnBeforeStartTransaction(
      request, wrapped_cb, headers);

  if (result == net::ERR_IO_PENDING)
    return result;

  return internal_callback.Run();
}

void AtomExtensionsNetworkDelegate::OnStartTransaction(
    net::URLRequest* request,
    const net::HttpRequestHeaders& headers) {
  atom::AtomNetworkDelegate::OnStartTransaction(request, headers);
  extensions_delegate_->OnStartTransaction(request, headers);
}

int AtomExtensionsNetworkDelegate::OnHeadersReceivedInternal(
    net::URLRequest* request,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {
  return atom::AtomNetworkDelegate::OnHeadersReceived(
                                          request,
                                          callbacks_[request->identifier()],
                                          original_response_headers,
                                          override_response_headers,
                                          allowed_unsafe_redirect_url);
}

int AtomExtensionsNetworkDelegate::OnHeadersReceived(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url) {

  callbacks_[request->identifier()] = callback;

  base::Callback<int(void)> internal_callback =
      base::Bind(&AtomExtensionsNetworkDelegate::OnHeadersReceivedInternal,
          base::Unretained(this),
          request,
          base::RetainedRef(original_response_headers),
          override_response_headers,
          allowed_unsafe_redirect_url);

  auto wrapped_cb = base::Bind(&AtomExtensionsNetworkDelegate::RunCallback,
                                base::Unretained(this),
                                internal_callback,
                                request->identifier());

  int result = extensions_delegate_->OnHeadersReceived(
      request,
      wrapped_cb,
      original_response_headers,
      override_response_headers,
      allowed_unsafe_redirect_url);

  if (result == net::OK)
    return internal_callback.Run();

  return result;
}

void AtomExtensionsNetworkDelegate::OnBeforeRedirect(
    net::URLRequest* request,
    const GURL& new_location) {
  atom::AtomNetworkDelegate::OnBeforeRedirect(request, new_location);
  extensions_delegate_->OnBeforeRedirect(request, new_location);
}

void AtomExtensionsNetworkDelegate::OnResponseStarted(
    net::URLRequest* request,
    int net_error) {
  atom::AtomNetworkDelegate::OnResponseStarted(request, net_error);
  extensions_delegate_->OnResponseStarted(request, net_error);
}

void AtomExtensionsNetworkDelegate::OnCompleted(
    net::URLRequest* request,
    bool started,
    int net_error) {
  callbacks_.erase(request->identifier());
  atom::AtomNetworkDelegate::OnCompleted(request, started, net_error);

  extensions_delegate_->OnCompleted(request, started, net_error);
  extensions_delegate_->ForwardProxyErrors(request, net_error);
  extensions_delegate_->ForwardDoneRequestStatus(request);
}

void AtomExtensionsNetworkDelegate::OnURLRequestDestroyed(
    net::URLRequest* request) {
  callbacks_.erase(request->identifier());
  atom::AtomNetworkDelegate::OnURLRequestDestroyed(request);
  extensions_delegate_->OnURLRequestDestroyed(request);
}

void AtomExtensionsNetworkDelegate::OnPACScriptError(
    int line_number,
    const base::string16& error) {
  extensions_delegate_->OnPACScriptError(line_number, error);
}

net::NetworkDelegate::AuthRequiredResponse
AtomExtensionsNetworkDelegate::OnAuthRequired(
    net::URLRequest* request,
    const net::AuthChallengeInfo& auth_info,
    const AuthCallback& callback,
    net::AuthCredentials* credentials) {
  return extensions_delegate_->OnAuthRequired(
      request, auth_info, callback, credentials);
}

}  // namespace extensions
