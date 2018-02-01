// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/atom_browser_client.h"
#include "extensions/features/features.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace content {
class PlatformNotificationService;
class NavigationHandle;
class RenderFrameHost;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace extensions {
class AtomBrowserClientExtensionsPart;
}
#endif

namespace blink {
struct WebWindowFeatures;
}  // namespace blink

namespace brave {

class BraveContentBrowserClient : public atom::AtomBrowserClient {
 public:
  BraveContentBrowserClient();
  virtual ~BraveContentBrowserClient();

  std::string GetAcceptLangs(content::BrowserContext* browser_context) override;
  std::string GetApplicationLocale() override;
  static void SetApplicationLocale(std::string locale);

 protected:
  // content::ContentBrowserClient:
  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;
  void BindInterfaceRequest(
      const service_manager::BindSourceInfo& source_info,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle* interface_pipe) override;
  void ExposeInterfacesToRenderer(
      service_manager::BinderRegistry* registry,
      blink::AssociatedInterfaceRegistry* associated_registry,
      content::RenderProcessHost* render_process_host) override;
  void RegisterInProcessServices(StaticServiceMap* services) override;
  void RegisterOutOfProcessServices(OutOfProcessServiceMap* services) override;
  void BindInterfaceRequestFromFrame(
      content::RenderFrameHost* render_frame_host,
      const std::string& interface_name,
      mojo::ScopedMessagePipeHandle interface_pipe) override;
#if defined(OS_POSIX) && !defined(OS_MACOSX)
  void GetAdditionalMappedFilesForChildProcess(
      const base::CommandLine& command_line,
      int child_process_id,
      content::PosixFileDescriptorInfo* mappings) override;
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)
  void RenderProcessWillLaunch(content::RenderProcessHost* host) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  content::PlatformNotificationService*
      GetPlatformNotificationService() override;
  void OverrideWebkitPrefs(content::RenderViewHost* host,
      content::WebPreferences* prefs) override;
  bool CanCreateWindow(content::RenderFrameHost* opener,
                       const GURL& opener_url,
                       const GURL& opener_top_level_frame_url,
                       const GURL& source_origin,
                       content::mojom::WindowContainerType container_type,
                       const GURL& target_url,
                       const content::Referrer& referrer,
                       const std::string& frame_name,
                       WindowOpenDisposition disposition,
                       const blink::mojom::WindowFeatures& features,
                       bool user_gesture,
                       bool opener_suppressed,
                       bool* no_javascript_access) override;
  GURL GetEffectiveURL(content::BrowserContext* browser_context,
                       const GURL& url) override;
  bool ShouldUseProcessPerSite(content::BrowserContext* browser_context,
                               const GURL& effective_url) override;
  bool DoesSiteRequireDedicatedProcess(content::BrowserContext* browser_context,
                                       const GURL& effective_site_url) override;
  bool ShouldLockToOrigin(content::BrowserContext* browser_context,
                          const GURL& effective_site_url) override;
  bool IsSuitableHost(content::RenderProcessHost* process_host,
                      const GURL& site_url) override;
  bool ShouldTryToUseExistingProcessHost(
      content::BrowserContext* browser_context,
      const GURL& url) override;

  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_allowed_schemes) override;
  void GetAdditionalWebUISchemes(
      std::vector<std::string>* additional_schemes) override;
  bool ShouldAllowOpenURL(content::SiteInstance* site_instance,
                                      const GURL& url) override;
  void BrowserURLHandlerCreated(
      content::BrowserURLHandler* handler) override;
  void SiteInstanceGotProcess(
      content::SiteInstance* site_instance) override;
  void SiteInstanceDeleting(
      content::SiteInstance* site_instance) override;
  bool ShouldSwapBrowsingInstancesForNavigation(
      content::SiteInstance* site_instance,
      const GURL& current_url,
      const GURL& new_url) override;
  std::string GetStoragePartitionIdForSite(
      content::BrowserContext* browser_context,
      const GURL& site) override;
  void GetStoragePartitionConfigForSite(
    content::BrowserContext* browser_context,
      const GURL& site,
      bool can_be_default,
      std::string* partition_domain,
      std::string* partition_name,
      bool* in_memory) override;
  base::FilePath GetShaderDiskCacheDirectory() override;

  std::unique_ptr<base::Value> GetServiceManifestOverlay(
      base::StringPiece name) override;


  std::vector<std::unique_ptr<content::NavigationThrottle>>
    CreateThrottlesForNavigation(
      content::NavigationHandle* handle) override;
  std::unique_ptr<content::NavigationUIData> GetNavigationUIData(
    content::NavigationHandle* navigation_handle) override;

 protected:
  bool IsValidStoragePartitionId(
    content::BrowserContext* browser_context,
    const std::string& partition_id);


 private:
  // Populate |frame_interfaces_|.
  void InitFrameInterfaces();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::unique_ptr<extensions::AtomBrowserClientExtensionsPart> extensions_part_;
#endif

  service_manager::BinderRegistry gpu_binder_registry_;
  std::unique_ptr<service_manager::BinderRegistry> frame_interfaces_;
  std::unique_ptr<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>
      frame_interfaces_parameterized_;

  DISALLOW_COPY_AND_ASSIGN(BraveContentBrowserClient);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_CONTENT_BROWSER_CLIENT_H_
