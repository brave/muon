// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brightray/browser/browser_client.h"
#include "content/public/browser/render_process_host_observer.h"
#include "net/cert/x509_certificate.h"
#include "net/ssl/client_cert_identity.h"
#include "net/ssl/ssl_private_key.h"


namespace content {
class QuotaPermissionContext;
class ClientCertificateDelegate;
}

namespace net {
class ClientCertIdentity;
using ClientCertIdentityList = std::vector<std::unique_ptr<ClientCertIdentity>>;
class SSLCertRequestInfo;
}

namespace atom {

class AtomResourceDispatcherHostDelegate;

class AtomBrowserClient : public brightray::BrowserClient,
                          public content::RenderProcessHostObserver {
 public:
  AtomBrowserClient();
  virtual ~AtomBrowserClient();

  using Delegate = content::ContentBrowserClient;
  void set_delegate(Delegate* delegate) { delegate_ = delegate; }

  // Returns the WebContents for pending render processes.
  content::WebContents* GetWebContentsFromProcessID(int process_id);

  // Custom schemes to be registered to handle service worker.
  static void SetCustomServiceWorkerSchemes(
      const std::vector<std::string>& schemes);

 protected:
  // content::ContentBrowserClient:
  content::SpeechRecognitionManagerDelegate*
      CreateSpeechRecognitionManagerDelegate() override;
  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;
  std::string GetApplicationLocale() override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  void DidCreatePpapiPlugin(content::BrowserPpapiHost* browser_host) override;
  content::QuotaPermissionContext* CreateQuotaPermissionContext() override;
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(
          content::CertificateRequestResultType)>& callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      net::ClientCertIdentityList client_certs,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;
  void ResourceDispatcherHostCreated() override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* schemes) override;

  // brightray::BrowserClient:
  brightray::BrowserMainParts* OverrideCreateBrowserMainParts(
      const content::MainFunctionParams&) override;
  void WebNotificationAllowed(
      int render_process_id,
      const base::Callback<void(bool, bool)>& callback) override;

  // content::RenderProcessHostObserver:
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;

 private:
  // pending_render_process => current_render_process.
  std::map<int, int> pending_processes_;

  Delegate* delegate_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserClient);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CLIENT_H_
