// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <utility>

#include "brave/browser/brave_content_browser_client.h"

#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/options_switches.h"
#include "base/base_switches.h"
#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/notifications/platform_notification_service_impl.h"
#include "brave/browser/password_manager/brave_password_manager_client.h"
#include "brave/grit/brave_resources.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/cache_stats_recorder.h"
#include "chrome/browser/chrome_service.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/plugins/plugin_info_host_impl.h"
#include "chrome/browser/printing/printing_message_filter.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/renderer_host/chrome_render_message_filter.h"
#include "chrome/browser/renderer_host/chrome_navigation_ui_data.h"
#include "chrome/browser/speech/tts_message_filter.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/constants.mojom.h"
#include "chrome/common/env_vars.h"
#include "chrome/common/importer/profile_import.mojom.h"
#include "chrome/common/render_messages.h"
#include "chrome/common/renderer_configuration.mojom.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents_view_delegate.h"
#include "content/public/browser/web_drag_dest_delegate.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/common/autofill_switches.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/password_manager/content/browser/content_password_manager_driver_factory.h"
#include "components/spellcheck/spellcheck_build_features.h"
#include "components/variations/variations_switches.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/service_names.mojom.h"
#include "content/public/common/url_constants.h"
#include "content/public/common/web_preferences.h"
#include "extensions/common/constants.h"
#include "gpu/config/gpu_switches.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "net/base/filename_util.h"
#include "services/data_decoder/public/interfaces/constants.mojom.h"
#include "services/metrics/metrics_mojo_service.h"
#include "services/metrics/public/interfaces/constants.mojom.h"
#include "services/proxy_resolver/public/interfaces/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/WebKit/public/web/WebWindowFeatures.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
#include "extensions/browser/extension_navigation_throttle.h"
#include "extensions/browser/extension_util.h"
#include "extensions/common/switches.h"
#endif

#if defined(USE_AURA)
#include "services/service_manager/runner/common/client_util.h"
#include "services/ui/public/cpp/gpu/gpu.h"
#include "ui/aura/mus/window_tree_client.h"
#include "ui/views/mus/mus_client.h"
#endif

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
#include "components/spellcheck/browser/spellcheck_message_filter_platform.h"
#endif

#if defined(OS_POSIX) && !defined(OS_MACOSX)
#include "base/debug/leak_annotations.h"
#include "components/crash/content/app/breakpad_linux.h"
#include "components/crash/content/browser/crash_handler_host_linux.h"
#endif

using content::BrowserThread;
using extensions::AtomBrowserClientExtensionsPart;

namespace brave {

namespace {

#if defined(OS_POSIX) && !defined(OS_MACOSX)
// TODO(bridiver) - move this to muon_crash_reporter_client
breakpad::CrashHandlerHostLinux* CreateCrashHandlerHost(
    const std::string& process_type) {
  base::FilePath dumps_path;
  PathService::Get(chrome::DIR_CRASH_DUMPS, &dumps_path);
  {
    ANNOTATE_SCOPED_MEMORY_LEAK;
    bool upload = (getenv(env_vars::kHeadless) == NULL);
    breakpad::CrashHandlerHostLinux* crash_handler =
        new breakpad::CrashHandlerHostLinux(process_type, dumps_path, upload);
    crash_handler->StartUploaderThread();
    return crash_handler;
  }
}

int GetCrashSignalFD(const base::CommandLine& command_line) {
  // Extensions have the same process type as renderers.
  if (command_line.HasSwitch(extensions::switches::kExtensionProcess)) {
    static breakpad::CrashHandlerHostLinux* crash_handler = NULL;
    if (!crash_handler)
      crash_handler = CreateCrashHandlerHost("extension");
    return crash_handler->GetDeathSignalSocket();
  }

  std::string process_type =
      command_line.GetSwitchValueASCII(switches::kProcessType);

  if (process_type == switches::kRendererProcess) {
    static breakpad::CrashHandlerHostLinux* crash_handler = NULL;
    if (!crash_handler)
      crash_handler = CreateCrashHandlerHost(process_type);
    return crash_handler->GetDeathSignalSocket();
  }

  if (process_type == switches::kPpapiPluginProcess) {
    static breakpad::CrashHandlerHostLinux* crash_handler = NULL;
    if (!crash_handler)
      crash_handler = CreateCrashHandlerHost(process_type);
    return crash_handler->GetDeathSignalSocket();
  }

  if (process_type == switches::kGpuProcess) {
    static breakpad::CrashHandlerHostLinux* crash_handler = NULL;
    if (!crash_handler)
      crash_handler = CreateCrashHandlerHost(process_type);
    return crash_handler->GetDeathSignalSocket();
  }

  return -1;
}
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

class BraveWebContentsViewDelegate : public content::WebContentsViewDelegate,
                                     public content::WebDragDestDelegate {
 public:
  explicit BraveWebContentsViewDelegate(content::WebContents* web_contents) :
      web_contents_(web_contents) {}
  ~BraveWebContentsViewDelegate() override {}
  content::WebDragDestDelegate* GetDragDestDelegate() override { return this; }
  void DragInitialize(content::WebContents* contents) override {}
  void OnDragOver() override {}
  void OnDragEnter() override {}
  void OnDragLeave() override {}
  void OnDrop() override {
    content::DropData* drop_data = web_contents_->GetDropData();

    if (!drop_data)
      return;

    if (web_contents_->GetWebUI()) {
      for (const auto& file_info : drop_data->filenames) {
        // add file path information for WebUI contexts in dataTransfer.items
        // the full path will be in `item.type` and the name is returned
        // by `getAsString`
        base::string16 file_url =
            base::UTF8ToUTF16(net::FilePathToFileURL(file_info.path).spec());
        drop_data->custom_data[file_url] =
            file_info.path.BaseName().LossyDisplayName();
      }
    }
  }
#if defined(USE_AURA)
  void OnReceiveDragData(const ui::OSExchangeData& data) override {};
#endif  // USE_AURA

 private:
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(BraveWebContentsViewDelegate);
};

// Cached version of the locale so we can return the locale on the I/O
// thread.
base::LazyInstance<std::string>::DestructorAtExit io_thread_application_locale;

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

content::WebContentsViewDelegate*
    BraveContentBrowserClient::GetWebContentsViewDelegate(
        content::WebContents* web_contents) {
  return new BraveWebContentsViewDelegate(web_contents);
}

void BraveContentBrowserClient::ExposeInterfacesToRenderer(
    service_manager::BinderRegistry* registry,
    blink::AssociatedInterfaceRegistry* associated_registry,
    content::RenderProcessHost* render_process_host) {
  // The CacheStatsRecorder is an associated binding, instead of a
  // non-associated one, because the sender (in the renderer process) posts the
  // message after a time delay, in order to rate limit. The association
  // protects against the render process host ID being recycled in that time
  // gap between the preparation and the execution of that IPC.
  associated_registry->AddInterface(
      base::Bind(&CacheStatsRecorder::Create, render_process_host->GetID()));

  scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner =
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI);

  Profile* profile =
      Profile::FromBrowserContext(render_process_host->GetBrowserContext());
  render_process_host->GetChannel()->AddAssociatedInterfaceForIOThread(
      base::Bind(&PluginInfoHostImpl::OnPluginInfoHostRequest,
                 base::MakeRefCounted<PluginInfoHostImpl>(
                     render_process_host->GetID(), profile)));
}

void BraveContentBrowserClient::RegisterInProcessServices(
    StaticServiceMap* services) {
  {
    service_manager::EmbeddedServiceInfo info;
    info.factory = base::Bind(&ChromeService::Create);
    services->insert(std::make_pair(chrome::mojom::kServiceName, info));
  }
  service_manager::EmbeddedServiceInfo info;
  info.factory = base::Bind(&metrics::CreateMetricsService);
  services->emplace(metrics::mojom::kMetricsServiceName, info);
}

void BraveContentBrowserClient::RegisterOutOfProcessServices(
    OutOfProcessServiceMap* services) {
  (*services)[chrome::mojom::kProfileImportServiceName] =
      l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_PROFILE_IMPORTER_NAME);

  (*services)[proxy_resolver::mojom::kProxyResolverServiceName] =
      l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_PROXY_RESOLVER_NAME);
}

void BraveContentBrowserClient::BindInterfaceRequest(
    const service_manager::BindSourceInfo& source_info,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle* interface_pipe) {
  if (source_info.identity.name() == content::mojom::kGpuServiceName &&
      gpu_binder_registry_.CanBindInterface(interface_name)) {
    gpu_binder_registry_.BindInterface(interface_name,
                                       std::move(*interface_pipe));
  }
}

void BraveContentBrowserClient::BindInterfaceRequestFromFrame(
    content::RenderFrameHost* render_frame_host,
    const std::string& interface_name,
    mojo::ScopedMessagePipeHandle interface_pipe) {
  if (!frame_interfaces_.get() && !frame_interfaces_parameterized_.get())
    InitFrameInterfaces();

  if (!frame_interfaces_parameterized_->TryBindInterface(
          interface_name, &interface_pipe, render_frame_host)) {
    frame_interfaces_->TryBindInterface(interface_name, &interface_pipe);
  }
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void BraveContentBrowserClient::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::PosixFileDescriptorInfo* mappings) {
  int crash_signal_fd = GetCrashSignalFD(command_line);
  if (crash_signal_fd >= 0) {
    mappings->Share(kCrashDumpSignal, crash_signal_fd);
  }
}
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

void BraveContentBrowserClient::RenderProcessWillLaunch(
    content::RenderProcessHost* host) {
  int id = host->GetID();
  Profile* profile = Profile::FromBrowserContext(host->GetBrowserContext());

  host->AddFilter(new printing::PrintingMessageFilter(id, profile));
  host->AddFilter(new TtsMessageFilter(host->GetBrowserContext()));

#if BUILDFLAG(USE_BROWSER_SPELLCHECKER)
  host->AddFilter(new SpellCheckMessageFilterPlatform(id));
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions_part_->RenderProcessWillLaunch(host);
#endif

  RendererContentSettingRules rules;
  GetRendererContentSettingRules(
    HostContentSettingsMapFactory::GetForProfile(profile), &rules);
  chrome::mojom::RendererConfigurationAssociatedPtr rc_interface;
  host->GetChannel()->GetRemoteAssociatedInterface(&rc_interface);
  rc_interface->SetContentSettingRules(rules);
  bool is_incognito_process = profile->IsOffTheRecord();
  rc_interface->SetInitialConfiguration(is_incognito_process);
}

GURL BraveContentBrowserClient::GetEffectiveURL(
    content::BrowserContext* browser_context,
    const GURL& url,
    bool is_isolated_origin) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (!profile)
    return url;

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // If |url| has an isolated origin, don't resolve effective URLs
  // corresponding to extensions, since isolated origins should take precedence
  // over hosted apps.  Note that for NTP, we do want to resolve the effective
  // URL above; see https://crbug.com/755595.
  if (is_isolated_origin)
    return url;

  return AtomBrowserClientExtensionsPart::GetEffectiveURL(
      profile, url, is_isolated_origin);
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

// static
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
  MuonCrashReporterClient::AppendExtraCommandLineSwitches(command_line);

  std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  const base::CommandLine& browser_command_line =
      *base::CommandLine::ForCurrentProcess();

  static const char* const kCommonSwitchNames[] = {
    switches::kUserAgent,
    // Make logs go to the right file.
    switches::kUserDataDir,
    atom::options::kUserDataDirName,
    atom::options::kAppVersion,  // version for renderer crash keys
    atom::options::kAppChannel,  // channel for renderer crash keys
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

      // TODO(bridiver) - change this if we enable chrome safebrowsing
      // Disable client-side phishing detection in the renderer if it is
      // disabled in the Profile preferences or the browser process.
      // if (!prefs->GetBoolean(prefs::kSafeBrowsingEnabled) ||
      //     !g_browser_process->safe_browsing_detection_service()) {
      command_line->AppendSwitch(
          switches::kDisableClientSidePhishingDetection);
      // }
    }

    static const char* const kSwitchNames[] = {
      autofill::switches::kDisablePasswordGeneration,
      autofill::switches::kEnablePasswordGeneration,
      autofill::switches::kEnableSingleClickAutofill,
      autofill::switches::kEnableSuggestionsWithSubstringMatch,
      autofill::switches::kIgnoreAutocompleteOffForAutofill,
      autofill::switches::kLocalHeuristicsOnlyForPasswordGeneration,
      autofill::switches::kShowAutofillSignatures,
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extensions::switches::kAllowHTTPBackgroundPage,
      extensions::switches::kAllowLegacyExtensionManifests,
      extensions::switches::kEnableEmbeddedExtensionOptions,
      extensions::switches::kEnableExperimentalExtensionApis,
      extensions::switches::kExtensionsOnChromeURLs,
      extensions::switches::kWhitelistedExtensionID,
#endif
      switches::kAllowInsecureLocalhost,
      switches::kDisableBundledPpapiFlash,
      switches::kDisableCastStreamingHWEncoding,
      switches::kDisableJavaScriptHarmonyShipping,
      variations::switches::kEnableBenchmarking,
      switches::kEnableNetBenchmarking,
      switches::kJavaScriptHarmony,
      switches::kPpapiFlashArgs,
      switches::kPpapiFlashPath,
      switches::kPpapiFlashVersion,
      switches::kProfilingAtStart,
      switches::kProfilingFile,
      switches::kProfilingFlush,
      switches::kUnsafelyTreatInsecureOriginAsSecure,
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
    content::RenderFrameHost* opener,
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
  additional_schemes->push_back(content::kChromeDevToolsScheme);
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
  PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  DCHECK(!user_data_dir.empty());
  return user_data_dir.Append(FILE_PATH_LITERAL("ShaderCache"));
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

std::unique_ptr<base::Value>
BraveContentBrowserClient::GetServiceManifestOverlay(base::StringPiece name) {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  int id = -1;
  if (name == content::mojom::kBrowserServiceName)
    id = IDR_CHROME_CONTENT_BROWSER_MANIFEST_OVERLAY;
  else if (name == content::mojom::kGpuServiceName)
    id = IDR_CHROME_CONTENT_GPU_MANIFEST_OVERLAY;
  else if (name == content::mojom::kPackagedServicesServiceName)
    id = IDR_CHROME_CONTENT_PACKAGED_SERVICES_MANIFEST_OVERLAY;
  else if (name == content::mojom::kPluginServiceName)
    id = IDR_CHROME_CONTENT_PLUGIN_MANIFEST_OVERLAY;
  else if (name == content::mojom::kRendererServiceName)
    id = IDR_CHROME_CONTENT_RENDERER_MANIFEST_OVERLAY;
  else if (name == content::mojom::kUtilityServiceName)
    id = IDR_CHROME_CONTENT_UTILITY_MANIFEST_OVERLAY;
  if (id == -1)
    return nullptr;

  base::StringPiece manifest_contents =
      rb.GetRawDataResourceForScale(id, ui::ScaleFactor::SCALE_FACTOR_NONE);
  return base::JSONReader::Read(manifest_contents);
}

std::vector<std::unique_ptr<content::NavigationThrottle>>
BraveContentBrowserClient::CreateThrottlesForNavigation(
    content::NavigationHandle* handle) {
  std::vector<std::unique_ptr<content::NavigationThrottle>> throttles;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (!handle->IsInMainFrame())
    throttles.push_back(
        base::MakeUnique<extensions::ExtensionNavigationThrottle>(handle));
#endif
  return throttles;
}

std::unique_ptr<content::NavigationUIData>
BraveContentBrowserClient::GetNavigationUIData(
    content::NavigationHandle* navigation_handle) {
  return base::MakeUnique<ChromeNavigationUIData>(navigation_handle);
}

void BraveContentBrowserClient::InitFrameInterfaces() {
  frame_interfaces_ = base::MakeUnique<service_manager::BinderRegistry>();
  frame_interfaces_parameterized_ = base::MakeUnique<
      service_manager::BinderRegistryWithArgs<content::RenderFrameHost*>>();

  frame_interfaces_parameterized_->AddInterface(
      base::Bind(&autofill::ContentAutofillDriverFactory::BindAutofillDriver));
  frame_interfaces_parameterized_->AddInterface(
      base::Bind(&password_manager::ContentPasswordManagerDriverFactory::
                     BindPasswordManagerDriver));
}

}  // namespace brave
