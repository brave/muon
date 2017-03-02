// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "brave/browser/brave_content_browser_client.h"

#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "base/base_switches.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "brave/browser/notifications/platform_notification_service_impl.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_render_message_filter.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/renderer_configuration.mojom.h"
#include "content/public/browser/storage_partition.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "extensions/common/constants.h"
#include "services/service_manager/public/cpp/interface_registry.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
#include "extensions/browser/extension_navigation_throttle.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/switches.h"
#endif

#if defined(USE_AURA)
#include "services/service_manager/runner/common/client_util.h"
#include "services/ui/public/cpp/gpu_service.h"
#include "ui/views/mus/window_manager_connection.h"
#endif


using content::BrowserThread;
using extensions::AtomBrowserClientExtensionsPart;

namespace brave {

namespace {

// Cached version of the locale so we can return the locale on the I/O
// thread.
base::LazyInstance<std::string> io_thread_application_locale;

void SetApplicationLocaleOnIOThread(const std::string& locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  io_thread_application_locale.Get() = locale;
}

}  // namespace

BraveContentBrowserClient::BraveContentBrowserClient() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_.reset(new AtomBrowserClientExtensionsPart);
#endif
}

BraveContentBrowserClient::~BraveContentBrowserClient() {
}

std::string BraveContentBrowserClient::GetStoragePartitionIdForSite(
    content::BrowserContext* browser_context,
    const GURL& site) {
  std::string partition_id;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (site.SchemeIs(extensions::kExtensionScheme) &&
           extensions::util::SiteHasIsolatedStorage(site, browser_context))
    partition_id = site.spec();
#endif

  DCHECK(IsValidStoragePartitionId(browser_context, partition_id));
  return partition_id;
}

bool BraveContentBrowserClient::IsValidStoragePartitionId(
    content::BrowserContext* browser_context,
    const std::string& partition_id) {
  // The default ID is empty and is always valid.
  if (partition_id.empty())
    return true;

  return GURL(partition_id).is_valid();
}

void BraveContentBrowserClient::GetStoragePartitionConfigForSite(
    content::BrowserContext* browser_context,
    const GURL& site,
    bool can_be_default,
    std::string* partition_domain,
    std::string* partition_name,
    bool* in_memory) {
  // Default to the browser-wide storage partition and override based on |site|
  // below.
  partition_domain->clear();
  partition_name->clear();
  *in_memory = false;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (site.SchemeIs(extensions::kExtensionScheme)) {
    // If |can_be_default| is false, the caller is stating that the |site|
    // should be parsed as if it had isolated storage. In particular it is
    // important to NOT check ExtensionService for the is_storage_isolated()
    // attribute because this code path is run during Extension uninstall
    // to do cleanup after the Extension has already been unloaded from the
    // ExtensionService.
    bool is_isolated = !can_be_default;
    if (can_be_default) {
      if (extensions::util::SiteHasIsolatedStorage(site, browser_context))
        is_isolated = true;
    }

    if (is_isolated) {
      CHECK(site.has_host());
      // For extensions with isolated storage, the the host of the |site| is
      // the |partition_domain|. The |in_memory| and |partition_name| are only
      // used in guest schemes so they are cleared here.
      *partition_domain = site.host();
      *in_memory = false;
      partition_name->clear();
    }
  }
#endif

  // Assert that if |can_be_default| is false, the code above must have found a
  // non-default partition.  If this fails, the caller has a serious logic
  // error about which StoragePartition they expect to be in and it is not
  // safe to continue.
  CHECK(can_be_default || !partition_domain->empty());
}

// TODO(bridiver) - investigate this
// content::WebContentsViewDelegate*
//     ChromeContentBrowserClient::GetWebContentsViewDelegate(
//         content::WebContents* web_contents) {
//   return chrome::CreateWebContentsViewDelegate(web_contents);
// }

void BraveContentBrowserClient::RegisterRenderFrameMojoInterfaces(
    service_manager::InterfaceRegistry* registry,
    content::RenderFrameHost* render_frame_host) {
  registry->AddInterface(
      base::Bind(&autofill::ContentAutofillDriverFactory::BindAutofillDriver,
                 render_frame_host));

  registry->AddInterface(
      base::Bind(&password_manager::ContentPasswordManagerDriverFactory::
                     BindPasswordManagerDriver,
                 render_frame_host));
}

void BraveContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());
  // initialize request context
  host->GetStoragePartition()->GetURLRequestContext();

  atom::AtomBrowserClient::RenderProcessWillLaunch(host);

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_->RenderProcessWillLaunch(host);
#endif

  RendererContentSettingRules rules;
  GetRendererContentSettingRules(
    HostContentSettingsMapFactory::GetForProfile(profile), &rules);
  chrome::mojom::RendererConfigurationAssociatedPtr rc_interface;
  host->GetChannel()->GetRemoteAssociatedInterface(&rc_interface);
  rc_interface->SetContentSettingRules(rules);
}

GURL BraveContentBrowserClient::GetEffectiveURL(
    content::BrowserContext* browser_context, const GURL& url) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile)
    return url;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  return AtomBrowserClientExtensionsPart::GetEffectiveURL(
      profile, url);
#else
  return url;
#endif
}

bool BraveContentBrowserClient::ShouldLockToOrigin(
    content::BrowserContext* browser_context,
    const GURL& effective_site_url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Disable origin lock if this is an extension/app that applies effective URL
  // mappings.
  if (!AtomBrowserClientExtensionsPart::ShouldLockToOrigin(
          browser_context, effective_site_url)) {
    return false;
  }
#endif
  return true;
}

bool BraveContentBrowserClient::IsSuitableHost(
    content::RenderProcessHost* process_host,
    const GURL& site_url) {
  Profile* profile =
      Profile::FromBrowserContext(process_host->GetBrowserContext());
  // This may be NULL during tests. In that case, just assume any site can
  // share any host.
  if (!profile)
    return true;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  return AtomBrowserClientExtensionsPart::IsSuitableHost(
      profile, process_host, site_url);
#else
  return true;
#endif
}

bool BraveContentBrowserClient::ShouldTryToUseExistingProcessHost(
    content::BrowserContext* browser_context, const GURL& url) {
  // It has to be a valid URL for us to check for an extension.
  if (!url.is_valid())
    return false;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return AtomBrowserClientExtensionsPart::
      ShouldTryToUseExistingProcessHost(
          profile, url);
#else
  return false;
#endif
}

void BraveContentBrowserClient::BrowserURLHandlerCreated(
    content::BrowserURLHandler* handler) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_->BrowserURLHandlerCreated(handler);
#endif
}

std::string BraveContentBrowserClient::GetAcceptLangs(
    content::BrowserContext* context) {
  // TODO(bridiver) - this should be set to all accept langs, not just
  // the default locale
  return GetApplicationLocale();
}

std::string BraveContentBrowserClient::GetApplicationLocale() {
  if (BrowserThread::CurrentlyOn(BrowserThread::IO)) {
    return io_thread_application_locale.Get();
  } else {
    return extensions_part_->GetApplicationLocale();
  }
}

void BraveContentBrowserClient::SetApplicationLocale(std::string locale) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
#if BUILDFLAG(ENABLE_EXTENSIONS)
  AtomBrowserClientExtensionsPart::SetApplicationLocale(locale);
#endif
  // This object is guaranteed to outlive all threads so we don't have to
  // worry about the lack of refcounting and can just post as Unretained.
  //
  // The common case is that this function is called early in Chrome startup
  // before any threads are created (it will also be called later if the user
  // changes the pref). In this case, there will be no threads created and
  // posting will fail. When there are no threads, we can just set the string
  // without worrying about threadsafety.
  if (!BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
          base::Bind(&SetApplicationLocaleOnIOThread, locale))) {
    io_thread_application_locale.Get() = locale;
  }
}

void BraveContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  atom::AtomBrowserClient::AppendExtraCommandLineSwitches(
      command_line, child_process_id);

#if defined(OS_POSIX) && !defined(OS_MACOSX)
//  if (breakpad::IsCrashReporterEnabled()) {
//    command_line->AppendSwitch(switches::kEnableCrashReporter);
//  }
#endif

  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();

  static const char* const kCommonSwitchNames[] = {
    switches::kUserAgent,
    switches::kUserDataDir,  // Make logs go to the right file.
  };
  command_line->CopySwitchesFrom(browser_command_line, kCommonSwitchNames,
                                 arraysize(kCommonSwitchNames));

  if (process_type == switches::kRendererProcess) {
    content::RenderProcessHost* process =
        content::RenderProcessHost::FromID(child_process_id);

    if (process) {
      #if BUILDFLAG(ENABLE_EXTENSIONS)
        extensions_part_->AppendExtraRendererCommandLineSwitches(
              command_line, process, process->GetBrowserContext());
      #endif
    }

    static const char* const kSwitchNames[] = {
      autofill::switches::kDisablePasswordGeneration,
      autofill::switches::kEnablePasswordGeneration,
      autofill::switches::kEnableSingleClickAutofill,
      autofill::switches::kEnableSuggestionsWithSubstringMatch,
      autofill::switches::kIgnoreAutocompleteOffForAutofill,
      autofill::switches::kLocalHeuristicsOnlyForPasswordGeneration,
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extensions::switches::kAllowHTTPBackgroundPage,
      extensions::switches::kAllowLegacyExtensionManifests,
      extensions::switches::kEnableEmbeddedExtensionOptions,
      extensions::switches::kEnableExperimentalExtensionApis,
      extensions::switches::kExtensionsOnChromeURLs,
      extensions::switches::kIsolateExtensions,
      extensions::switches::kWhitelistedExtensionID,
#endif
    };
    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));
  } else if (process_type == switches::kUtilityProcess) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
    static const char* const kSwitchNames[] = {
      extensions::switches::kAllowHTTPBackgroundPage,
      extensions::switches::kEnableExperimentalExtensionApis,
      extensions::switches::kExtensionsOnChromeURLs,
      extensions::switches::kWhitelistedExtensionID,
    };

    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));
#endif
  } else if (process_type == switches::kZygoteProcess) {
    static const char* const kSwitchNames[] = {
      // Load (in-process) Pepper plugins in-process in the zygote pre-sandbox.
      switches::kDisableBundledPpapiFlash,
      switches::kPpapiFlashPath,
      switches::kPpapiFlashVersion,
    };

    command_line->CopySwitchesFrom(browser_command_line, kSwitchNames,
                                   arraysize(kSwitchNames));
  } else if (process_type == switches::kGpuProcess) {
    // If --ignore-gpu-blacklist is passed in, don't send in crash reports
    // because GPU is expected to be unreliable.
    if (browser_command_line.HasSwitch(switches::kIgnoreGpuBlacklist) &&
        !command_line->HasSwitch(switches::kDisableBreakpad))
      command_line->AppendSwitch(switches::kDisableBreakpad);
  }
}

bool BraveContentBrowserClient::CanCreateWindow(
    int opener_render_process_id,
    int opener_render_frame_id,
    const GURL& opener_url,
    const GURL& opener_top_level_frame_url,
    const GURL& source_origin,
    WindowContainerType container_type,
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const blink::WebWindowFeatures& features,
    bool user_gesture,
    bool opener_suppressed,
    content::ResourceContext* context,
    bool* no_javascript_access) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  *no_javascript_access = false;

  // always return true because we'll block popups later
  return true;
}

content::PlatformNotificationService*
BraveContentBrowserClient::GetPlatformNotificationService() {
  return PlatformNotificationServiceImpl::GetInstance();
}

void BraveContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* host, content::WebPreferences* prefs) {
  prefs->hyperlink_auditing_enabled = false;
  // Custom preferences of guest page.
  auto web_contents = content::WebContents::FromRenderViewHost(host);
  atom::WebContentsPreferences::OverrideWebkitPrefs(web_contents, prefs);
  #if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions_part_->OverrideWebkitPrefs(host, prefs);
  #endif
}


void BraveContentBrowserClient::SiteInstanceGotProcess(
    content::SiteInstance* site_instance) {
  DCHECK(site_instance->HasProcess());

  auto browser_context = site_instance->GetBrowserContext();
  if (!browser_context)
    return;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_->SiteInstanceGotProcess(site_instance);
#endif
}

void BraveContentBrowserClient::SiteInstanceDeleting(
    content::SiteInstance* site_instance) {
  if (!site_instance->HasProcess())
    return;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_->SiteInstanceDeleting(site_instance);
#endif
}

bool BraveContentBrowserClient::ShouldUseProcessPerSite(
    content::BrowserContext* browser_context, const GURL& effective_url) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile)
    return false;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  return AtomBrowserClientExtensionsPart::ShouldUseProcessPerSite(
      profile, effective_url);
#else
  return false;
#endif
}

bool BraveContentBrowserClient::DoesSiteRequireDedicatedProcess(
    content::BrowserContext* browser_context,
    const GURL& effective_site_url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (AtomBrowserClientExtensionsPart::DoesSiteRequireDedicatedProcess(
          browser_context, effective_site_url)) {
    return true;
  }
#endif
  return false;
}

void BraveContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
        std::vector<std::string>* additional_allowed_schemes) {
  AtomBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
      additional_allowed_schemes);
  ContentBrowserClient::GetAdditionalAllowedSchemesForFileSystem(
      additional_allowed_schemes);

  additional_allowed_schemes->push_back(content::kChromeDevToolsScheme);
  additional_allowed_schemes->push_back(content::kChromeUIScheme);

  #if BUILDFLAG(ENABLE_EXTENSIONS)
    extensions_part_->
        GetAdditionalAllowedSchemesForFileSystem(additional_allowed_schemes);
  #endif
}

void BraveContentBrowserClient::GetAdditionalWebUISchemes(
    std::vector<std::string>* additional_schemes) {
}

bool BraveContentBrowserClient::ShouldAllowOpenURL(
    content::SiteInstance* site_instance, const GURL& url) {
  GURL from_url = site_instance->GetSiteURL();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool result;
  if (AtomBrowserClientExtensionsPart::ShouldAllowOpenURL(
      site_instance, from_url, url, &result))
    return result;
#endif

  return true;
}

base::FilePath BraveContentBrowserClient::GetShaderDiskCacheDirectory() {
  base::FilePath user_data_dir;
  PathService::Get(brightray::DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());
  return user_data_dir.Append(FILE_PATH_LITERAL("ShaderCache"));
}

gpu::GpuChannelEstablishFactory*
BraveContentBrowserClient::GetGpuChannelEstablishFactory() {
#if defined(USE_AURA)
  if (views::WindowManagerConnection::Exists())
    return views::WindowManagerConnection::Get()->gpu_service();
#endif
  return nullptr;
}

void BraveContentBrowserClient::RegisterInProcessServices(
    StaticServiceMap* apps) {
#if (ENABLE_MOJO_MEDIA_IN_BROWSER_PROCESS)
  content::MojoApplicationInfo app_info;
  app_info.application_factory = base::Bind(&media::CreateMojoMediaApplication);
  apps->insert(std::make_pair("mojo:media", app_info));
#endif
}

void BraveContentBrowserClient::RegisterOutOfProcessServices(
      OutOfProcessServiceMap* apps) {
#if defined(ENABLE_MOJO_MEDIA_IN_UTILITY_PROCESS)
  apps->insert(std::make_pair("mojo:media",
                              base::ASCIIToUTF16("Media App")));
#endif
}

bool BraveContentBrowserClient::ShouldSwapBrowsingInstancesForNavigation(
    content::SiteInstance* site_instance,
    const GURL& current_url,
    const GURL& new_url) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return AtomBrowserClientExtensionsPart::
      ShouldSwapBrowsingInstancesForNavigation(
          site_instance, current_url, new_url);
#else
  return false;
#endif
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BraveContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!handle->IsInMainFrame())
    throttles.push_back(
      base::MakeUnique<content::NavigationThrottle>(
          extensions::ExtensionNavigationThrottle(handle)));
#endif
  return throttles;
}

}  // namespace brave
