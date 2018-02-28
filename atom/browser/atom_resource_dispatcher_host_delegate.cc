// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/login_handler.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/common/platform_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/loader/safe_browsing_resource_throttle.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/offline_pages/features/features.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_request_info.h"

#include "net/base/escape.h"
#include "net/ssl/client_cert_store.h"
#include "url/gurl.h"

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#endif

using content::BrowserThread;
using content::ResourceRequestInfo;
using content::ResourceType;

namespace atom {

namespace {

void OnOpenExternal(const GURL& escaped_url,
                    bool allowed) {
  if (allowed)
    platform_util::OpenExternal(
#if defined(OS_WIN)
        base::UTF8ToUTF16(escaped_url.spec()),
#else
        escaped_url,
#endif
        true);
}

void HandleExternalProtocolInUI(
    const GURL& url,
    const ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::Bind(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(callback, has_user_gesture);
}

}  // namespace

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() :
  safe_browsing_(g_browser_process->safe_browsing_service()) {
}

void AtomResourceDispatcherHostDelegate::RequestBeginning(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    content::AppCacheService* appcache_service,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {
  if (safe_browsing_.get())
    safe_browsing_->OnResourceRequest(request);

  const ResourceRequestInfo* info = ResourceRequestInfo::ForRequest(request);

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&NotifyUIThreadOfRequestStarted,
                                         info->GetWebContentsGetterForRequest(),
                                         info->GetResourceType()));
#endif  // BUILDFLAG(ENABLE_OFFLINE_PAGES)

  if (!request->is_pending()) {
    net::HttpRequestHeaders headers;
    headers.CopyFrom(request->extra_request_headers());
    request->SetExtraRequestHeaders(headers);
  }

  AppendStandardResourceThrottles(request,
                                  resource_context,
                                  resource_type,
                                  throttles);
#if BUILDFLAG(ENABLE_NACL)
  AppendComponentUpdaterThrottles(request, *info, resource_context,
                                  resource_type, throttles);
#endif  // BUILDFLAG(ENABLE_NACL)
}

void AtomResourceDispatcherHostDelegate::AppendStandardResourceThrottles(
    net::URLRequest* request,
    content::ResourceContext* resource_context,
    ResourceType resource_type,
    std::vector<std::unique_ptr<content::ResourceThrottle>>* throttles) {

  content::ResourceThrottle* first_throttle = NULL;

#if defined(SAFE_BROWSING_DB_LOCAL) || defined(SAFE_BROWSING_DB_REMOTE)
  if (!first_throttle) {
    first_throttle = MaybeCreateSafeBrowsingResourceThrottle(
        request, resource_type, safe_browsing_.get());
  }
#endif  // defined(SAFE_BROWSING_DB_LOCAL) || defined(SAFE_BROWSING_DB_REMOTE)

  if (first_throttle)
    throttles->push_back(base::WrapUnique(first_throttle));
}

bool AtomResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    ResourceRequestInfo* info) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&HandleExternalProtocolInUI,
                                     url,
                                     info->GetWebContentsGetterForRequest(),
                                     info->HasUserGesture()));
  return true;
}

content::ResourceDispatcherHostLoginDelegate*
AtomResourceDispatcherHostDelegate::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    net::URLRequest* request) {
  return new LoginHandler(auth_info, request);
}

std::unique_ptr<net::ClientCertStore>
AtomResourceDispatcherHostDelegate::CreateClientCertStore(
    content::ResourceContext* resource_context) {
  #if defined(USE_NSS_CERTS)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreNSS(
        net::ClientCertStoreNSS::PasswordDelegateFactory()));
  #elif defined(OS_WIN)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreWin());
  #elif defined(OS_MACOSX)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreMac());
  #elif defined(USE_OPENSSL)
    return std::unique_ptr<net::ClientCertStore>();
  #endif
}

}  // namespace atom
