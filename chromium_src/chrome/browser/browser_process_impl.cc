// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_impl.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "brave/browser/resource_coordinator/guest_tab_manager.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/background/background_mode_manager.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/status_icons/status_tray.h"
#include "chrome/common/chrome_paths.h"
#include "components/component_updater/component_updater_service.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/sync_preferences/pref_service_syncable_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "ppapi/features/features.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
#include "chrome/common/extensions/extension_process_policy.h"
#include "chrome/common/extensions/chrome_extensions_client.h"
#include "extensions/common/constants.h"
#include "extensions/common/features/feature_provider.h"
#include "ui/base/resource/resource_bundle.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "brave/browser/plugins/brave_plugin_service_filter.h"
#include "chrome/browser/plugins/plugin_finder.h"
#include "content/public/browser/plugin_service.h"
#endif

using content::ChildProcessSecurityPolicy;
using content::PluginService;

BrowserProcessImpl::BrowserProcessImpl(
      base::SequencedTaskRunner* local_state_task_runner) :
    local_state_task_runner_(local_state_task_runner),
    tearing_down_(false),
    created_profile_manager_(false),
    created_local_state_(false),
    locale_(l10n_util::GetApplicationLocale("")) {
  g_browser_process = this;

  print_job_manager_.reset(new printing::PrintJobManager);

#if defined(OS_MACOSX)
  ui::InitIdleMonitor();
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_event_router_forwarder_ = new extensions::EventRouterForwarder;

  extensions::ExtensionsClient::Set(
      extensions::ChromeExtensionsClient::GetInstance());
  extensions_browser_client_.reset(
      new extensions::AtomExtensionsBrowserClient());
  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());
#endif

  message_center::MessageCenter::Initialize();
}

BrowserProcessImpl::~BrowserProcessImpl() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionsBrowserClient::Set(nullptr);
#endif
  g_browser_process = NULL;
}

void BrowserProcessImpl::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && !profile_manager_);
  created_profile_manager_ = true;

  base::FilePath user_data_dir;
  PathService::Get(brightray::DIR_USER_DATA, &user_data_dir);
  profile_manager_.reset(new ProfileManager(user_data_dir));
}

ProfileManager* BrowserProcessImpl::profile_manager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!created_profile_manager_)
    CreateProfileManager();
  return profile_manager_.get();
}

rappor::RapporServiceImpl* BrowserProcessImpl::rappor_service() {
  return nullptr;
}

ukm::UkmRecorder* BrowserProcessImpl::ukm_recorder() {
  return nullptr;
}

component_updater::ComponentUpdateService*
BrowserProcessImpl::component_updater(
    std::unique_ptr<component_updater::ComponentUpdateService> &component_updater,
    bool use_brave_server) {
  if (!component_updater.get()) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
      return NULL;
    Profile* profile = ProfileManager::GetPrimaryUserProfile();
    scoped_refptr<update_client::Configurator> configurator =
        component_updater::MakeBraveComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess(),
            profile->GetRequestContext(),
            use_brave_server);
    // Creating the component updater does not do anything, components
    // need to be registered and Start() needs to be called.
    component_updater.reset(component_updater::ComponentUpdateServiceFactory(
                                 configurator).release());
  }
  return component_updater.get();
}

component_updater::ComponentUpdateService*
BrowserProcessImpl::brave_component_updater() {
  return component_updater(brave_component_updater_, true);
}

component_updater::ComponentUpdateService*
BrowserProcessImpl::component_updater() {
  return component_updater(component_updater_, false);
}

extensions::EventRouterForwarder*
BrowserProcessImpl::extension_event_router_forwarder() {
  return extension_event_router_forwarder_.get();
}

message_center::MessageCenter* BrowserProcessImpl::message_center() {
  DCHECK(thread_checker_.CalledOnValidThread());
  return message_center::MessageCenter::Get();
}

void BrowserProcessImpl::StartTearDown() {
  tearing_down_ = true;
  browser_shutdown::SetTryingToQuit(true);

  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    i.GetCurrentValue()->FastShutdownIfPossible();
  }

  print_job_manager_->Shutdown();

  profile_manager_.reset();

  message_center::MessageCenter::Shutdown();
}

void BrowserProcessImpl::SetApplicationLocale(const std::string& locale) {
  locale_ = locale;
  brave::BraveContentBrowserClient::SetApplicationLocale(locale);
}

const std::string& BrowserProcessImpl::GetApplicationLocale() {
  DCHECK(!locale_.empty());
  return locale_;
}

printing::PrintJobManager* BrowserProcessImpl::print_job_manager() {
  return print_job_manager_.get();
}

void BrowserProcessImpl::CreateLocalState() {
  DCHECK(!created_local_state_ && !local_state_);
  created_local_state_ = true;

  base::FilePath local_state_path;
  CHECK(PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path));
  scoped_refptr<PrefRegistrySimple> pref_registry = new PrefRegistrySimple;

#if defined(OS_WIN)
    password_manager::PasswordManager::RegisterLocalPrefs(pref_registry.get());
#endif

  sync_preferences::PrefServiceSyncableFactory factory;
  factory.set_async(false);
  factory.set_user_prefs(
      new JsonPrefStore(local_state_path,
                        local_state_task_runner_.get(),
                        std::unique_ptr<PrefFilter>()));
  local_state_ = factory.Create(pref_registry.get());
}

bool BrowserProcessImpl::created_local_state() const {
  return created_local_state_;
}

PrefService* BrowserProcessImpl::local_state() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!created_local_state_)
    CreateLocalState();
  return local_state_.get();
}

bool BrowserProcessImpl::IsShuttingDown() {
  return tearing_down_;
}

void BrowserProcessImpl::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new atom::AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}

void BrowserProcessImpl::PreCreateThreads() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Disable Isolate Extensions by default
  ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      extensions::kExtensionScheme);
#endif
}

void BrowserProcessImpl::PreMainMessageLoopRun() {
#if BUILDFLAG(ENABLE_PLUGINS)
  PluginService* plugin_service = PluginService::GetInstance();
  plugin_service->SetFilter(BravePluginServiceFilter::GetInstance());

  // Triggers initialization of the singleton instance on UI thread.
  PluginFinder::GetInstance()->Init();
#endif  // BUILDFLAG(ENABLE_PLUGINS)

// Start the tab manager here so that we give the most amount of time for the
// other services to start up before we start adjusting the oom priority.
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  g_browser_process->GetTabManager()->Start();
#endif
}

resource_coordinator::TabManager* BrowserProcessImpl::GetTabManager() {
  DCHECK(thread_checker_.CalledOnValidThread());
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  if (!tab_manager_.get())
    tab_manager_.reset(new resource_coordinator::GuestTabManager());
  return tab_manager_.get();
#else
  return nullptr;
#endif
}

// NOTIMPLEMENTED
void BrowserProcessImpl::EndSession() {
  NOTIMPLEMENTED();
}

net_log::ChromeNetLog* BrowserProcessImpl::net_log() {
  NOTIMPLEMENTED();
  return nullptr;
}

metrics_services_manager::MetricsServicesManager*
BrowserProcessImpl::GetMetricsServicesManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

net::URLRequestContextGetter* BrowserProcessImpl::system_request_context() {
  NOTIMPLEMENTED();
  return nullptr;
}

variations::VariationsService* BrowserProcessImpl::variations_service() {
  NOTIMPLEMENTED();
  return nullptr;
}

BrowserProcessPlatformPart* BrowserProcessImpl::platform_part() {
  NOTIMPLEMENTED();
  return nullptr;
}

NotificationUIManager* BrowserProcessImpl::notification_ui_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

NotificationPlatformBridge* BrowserProcessImpl::notification_platform_bridge() {
  NOTIMPLEMENTED();
  return nullptr;
}

IOThread* BrowserProcessImpl::io_thread() {
  NOTIMPLEMENTED();
  return nullptr;
}

WatchDogThread* BrowserProcessImpl::watchdog_thread() {
  NOTIMPLEMENTED();
  return nullptr;
}

policy::BrowserPolicyConnector* BrowserProcessImpl::browser_policy_connector() {
  NOTIMPLEMENTED();
  return nullptr;
}

policy::PolicyService* BrowserProcessImpl::policy_service() {
  NOTIMPLEMENTED();
  return nullptr;
}

IconManager* BrowserProcessImpl::icon_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

GpuModeManager* BrowserProcessImpl::gpu_mode_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

GpuProfileCache* BrowserProcessImpl::gpu_profile_cache() {
  NOTIMPLEMENTED();
  return nullptr;
}

void BrowserProcessImpl::CreateDevToolsHttpProtocolHandler(
    const std::string& ip, uint16_t port) {
  NOTIMPLEMENTED();
}

void BrowserProcessImpl::CreateDevToolsAutoOpener() {
  NOTIMPLEMENTED();
}

printing::PrintPreviewDialogController*
BrowserProcessImpl::print_preview_dialog_controller() {
  NOTIMPLEMENTED();
  return nullptr;
}

printing::BackgroundPrintingManager*
BrowserProcessImpl::background_printing_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

IntranetRedirectDetector* BrowserProcessImpl::intranet_redirect_detector() {
  NOTIMPLEMENTED();
  return nullptr;
}

DownloadStatusUpdater* BrowserProcessImpl::download_status_updater() {
  NOTIMPLEMENTED();
  return nullptr;
}

DownloadRequestLimiter* BrowserProcessImpl::download_request_limiter() {
  NOTIMPLEMENTED();
  return nullptr;
}

BackgroundModeManager* BrowserProcessImpl::background_mode_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

void BrowserProcessImpl::set_background_mode_manager_for_test(
    std::unique_ptr<BackgroundModeManager> manager) {
  NOTIMPLEMENTED();
}

safe_browsing::SafeBrowsingService*
BrowserProcessImpl::safe_browsing_service() {
  return nullptr;
}

safe_browsing::ClientSideDetectionService*
BrowserProcessImpl::safe_browsing_detection_service() {
  return nullptr;
}

subresource_filter::ContentRulesetService*
BrowserProcessImpl::subresource_filter_ruleset_service() {
  NOTIMPLEMENTED();
  return nullptr;
}

CRLSetFetcher* BrowserProcessImpl::crl_set_fetcher() {
  NOTIMPLEMENTED();
  return nullptr;
}

component_updater::PnaclComponentInstaller*
BrowserProcessImpl::pnacl_component_installer() {
  NOTIMPLEMENTED();
  return nullptr;
}

component_updater::SupervisedUserWhitelistInstaller*
BrowserProcessImpl::supervised_user_whitelist_installer() {
  NOTIMPLEMENTED();
  return nullptr;
}

MediaFileSystemRegistry* BrowserProcessImpl::media_file_system_registry() {
  NOTIMPLEMENTED();
  return nullptr;
}

#if BUILDFLAG(ENABLE_WEBRTC)
WebRtcLogUploader* BrowserProcessImpl::webrtc_log_uploader() {
  NOTIMPLEMENTED();
  return nullptr;
}

#endif
network_time::NetworkTimeTracker* BrowserProcessImpl::network_time_tracker() {
  NOTIMPLEMENTED();
  return nullptr;
}

gcm::GCMDriver* BrowserProcessImpl::gcm_driver() {
  NOTIMPLEMENTED();
  return nullptr;
}

StatusTray* BrowserProcessImpl::status_tray() {
  NOTIMPLEMENTED();
  return nullptr;
}

shell_integration::DefaultWebClientState
BrowserProcessImpl::CachedDefaultWebClientState() {
  NOTIMPLEMENTED();
  return shell_integration::UNKNOWN_DEFAULT;
}

physical_web::PhysicalWebDataSource*
BrowserProcessImpl::GetPhysicalWebDataSource() {
  NOTIMPLEMENTED();
  return nullptr;
}

#if (defined(OS_WIN) || defined(OS_LINUX)) && !defined(OS_CHROMEOS)
void BrowserProcessImpl::StartAutoupdateTimer() {
  NOTIMPLEMENTED();
}
#endif
