// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process_impl.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/metrics/field_trial.h"
#include "base/path_service.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "base/trace_event/trace_event.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/resource_coordinator/guest_tab_manager.h"
#include "chrome/browser/background/background_mode_manager.h"
#include "chrome/browser/browser_shutdown.h"
#include "chrome/browser/component_updater/crl_set_component_installer.h"
#include "chrome/browser/metrics/chrome_metrics_service_accessor.h"
#include "chrome/browser/notifications/notification_ui_manager_stub.h"
#include "chrome/browser/prefs/chrome_command_line_pref_store.h"
#include "chrome/browser/printing/background_printing_manager.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/status_icons/status_tray.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/installer/util/google_update_settings.h"
#include "components/component_updater/component_updater_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/optimization_guide/optimization_guide_service.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/policy/core/common/policy_service_stub.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/ssl_config/ssl_config_service_manager.h"
#include "components/sync_preferences/pref_service_syncable_factory.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/network_connection_tracker.h"
#include "ppapi/features/features.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"

// these are just to satisfy build errors for unused vars in
// browser_process_imp.h
// they should be moved above if/when they are actually used
#include "chrome/browser/chrome_child_process_watcher.h"
#include "chrome/browser/chrome_device_client.h"
#include "chrome/browser/devtools/remote_debugging_server.h"
#include "chrome/browser/gpu/gpu_mode_manager.h"
#include "chrome/browser/gpu/gpu_profile_cache.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/browser/intranet_redirect_detector.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/media/webrtc/webrtc_log_uploader.h"
#include "chrome/browser/media_galleries/media_file_system_registry.h"
#include "chrome/browser/metrics/thread_watcher.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"
#include "chrome/browser/plugins/plugins_resource_service.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/gcm_driver/gcm_driver.h"
#include "components/network_time/network_time_tracker.h"
#include "components/net_log/chrome_net_log.h"
#include "components/physical_web/data_source/physical_web_data_source.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/subresource_filter/content/browser/content_ruleset_service.h"
#include "services/preferences/public/cpp/in_process_service_factory.h"
#include "chrome/browser/component_updater/supervised_user_whitelist_installer.h"
#include "chrome/browser/devtools/devtools_auto_opener.h"
#include "chrome/browser/download/download_request_limiter.h"
#include "chrome/browser/download/download_status_updater.h"
#include "chrome/browser/loader/chrome_resource_dispatcher_host_delegate.h"
#include "chrome/browser/printing/print_preview_dialog_controller.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
#include "chrome/common/extensions/extension_process_policy.h"
#include "chrome/common/extensions/chrome_extensions_client.h"
#include "components/storage_monitor/storage_monitor.h"
#include "extensions/common/constants.h"
#include "extensions/common/features/feature_provider.h"
#include "ui/base/resource/resource_bundle.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "brave/browser/plugins/brave_plugin_service_filter.h"
#include "chrome/browser/plugins/plugin_finder.h"
#include "content/public/browser/plugin_service.h"
#endif

#if defined(OS_WIN)
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/install_util.h"

// Type for the function pointer to enable and disable crash reporting on
// windows. Needed because the function is loaded from chrome_elf.
typedef void (*SetUploadConsentPointer)(bool);

// The name of the function used to set the uploads enabled state in
// components/crash/content/app/crashpad.cc. This is used to call the function
// exported by the chrome_elf dll.
const char kCrashpadUpdateConsentFunctionName[] = "SetUploadConsentImpl";
#endif  // OS_WIN

#if defined(USE_X11) || defined(OS_WIN) || defined(USE_OZONE)
// How long to wait for the File thread to complete during EndSession, on Linux
// and Windows. We have a timeout here because we're unable to run the UI
// messageloop and there's some deadlock risk. Our only option is to exit
// anyway.
static constexpr base::TimeDelta kEndSessionTimeout =
    base::TimeDelta::FromSeconds(10);
#endif

using content::ChildProcessSecurityPolicy;
using content::PluginService;

namespace {
base::LazyInstance<policy::PolicyServiceStub>::Leaky policy_service_stub =
    LAZY_INSTANCE_INITIALIZER;
}  // namespace

BrowserProcessImpl::BrowserProcessImpl(
      base::SequencedTaskRunner* local_state_task_runner) :
    created_watchdog_thread_(false),
    created_browser_policy_connector_(false),
    created_profile_manager_(false),
    created_icon_manager_(false),
    created_notification_ui_manager_(false),
    created_notification_bridge_(false),
    created_safe_browsing_service_(false),
    created_subresource_filter_ruleset_service_(false),
    shutting_down_(false),
    tearing_down_(false),
    locale_(l10n_util::GetApplicationLocale("")),
    local_state_task_runner_(local_state_task_runner) {
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

  net_log_ = base::MakeUnique<net_log::ChromeNetLog>();

  // TODO(darkdh): support this later
  // if (command_line.HasSwitch(switches::kLogNetLog)) {
  //   net_log_->StartWritingToFile(
  //       command_line.GetSwitchValuePath(switches::kLogNetLog),
  //       GetNetCaptureModeFromCommandLine(command_line),
  //       command_line.GetCommandLineString(), chrome::GetChannelString());
  // }
}

BrowserProcessImpl::~BrowserProcessImpl() {
  pref_change_registrar_.RemoveAll();
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionsBrowserClient::Set(nullptr);
#endif
  g_browser_process = NULL;
}

void BrowserProcessImpl::PostDestroyThreads() {

  // This observes |local_state_|, so should be destroyed before it.
  system_network_context_manager_.reset();

  // Reset associated state right after actual thread is stopped,
  // as io_thread_.global_ cleanup happens in CleanUp on the IO
  // thread, i.e. as the thread exits its message loop.
  //
  // This is important also because in various places, the
  // IOThread object being NULL is considered synonymous with the
  // IO thread having stopped.
  io_thread_.reset();
}

void BrowserProcessImpl::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && !profile_manager_);
  created_profile_manager_ = true;

  base::FilePath user_data_dir;
  PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  profile_manager_.reset(new ProfileManager(user_data_dir));
}

ProfileManager* BrowserProcessImpl::profile_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!created_profile_manager_)
    CreateProfileManager();
  return profile_manager_.get();
}

rappor::RapporServiceImpl* BrowserProcessImpl::rappor_service() {
  return nullptr;
}

extensions::EventRouterForwarder*
BrowserProcessImpl::extension_event_router_forwarder() {
  return extension_event_router_forwarder_.get();
}

message_center::MessageCenter* BrowserProcessImpl::message_center() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return message_center::MessageCenter::Get();
}

void BrowserProcessImpl::StartTearDown() {
  TRACE_EVENT0("shutdown", "BrowserProcessImpl::StartTearDown");
  // TODO(crbug.com/560486): Fix the tests that make the check of
  // |tearing_down_| necessary in IsShuttingDown().
  tearing_down_ = true;
  DCHECK(IsShuttingDown());

  browser_shutdown::SetTryingToQuit(true);
 
  if (safe_browsing_service_.get())
    safe_browsing_service()->ShutDown();

  for (content::RenderProcessHost::iterator i(
          content::RenderProcessHost::AllHostsIterator());
       !i.IsAtEnd(); i.Advance()) {
    i.GetCurrentValue()->FastShutdownIfPossible();
  }

  print_job_manager_->Shutdown();

  profile_manager_.reset();

  message_center::MessageCenter::Shutdown();

  if (local_state()) {
    local_state()->CommitPendingWrite();
  }
}

safe_browsing::SafeBrowsingService* 
    BrowserProcessImpl::safe_browsing_service() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!created_safe_browsing_service_)
    CreateSafeBrowsingService();
  return safe_browsing_service_.get();
}

safe_browsing::ClientSideDetectionService*
    BrowserProcessImpl::safe_browsing_detection_service() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (safe_browsing_service())
    return safe_browsing_service()->safe_browsing_detection_service();
  return NULL;
}

void BrowserProcessImpl::CreateSafeBrowsingService() {
  DCHECK(!safe_browsing_service_);
  created_safe_browsing_service_ = true;
  safe_browsing_service_ =
      safe_browsing::SafeBrowsingService::CreateSafeBrowsingService();
  safe_browsing_service_->Initialize();
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
  DCHECK(!local_state_);

  base::FilePath local_state_path;
  CHECK(PathService::Get(chrome::FILE_LOCAL_STATE, &local_state_path));
  scoped_refptr<PrefRegistrySimple> pref_registry = new PrefRegistrySimple;

#if defined(OS_WIN)
  password_manager::PasswordManager::RegisterLocalPrefs(pref_registry.get());
#endif
  pref_registry->RegisterBooleanPref(
      metrics::prefs::kMetricsReportingEnabled, false);

  sync_preferences::PrefServiceSyncableFactory factory;
  factory.set_async(false);
  factory.set_user_prefs(
      new JsonPrefStore(local_state_path,
                        local_state_task_runner_.get(),
                        std::unique_ptr<PrefFilter>()));
  local_state_ = factory.Create(pref_registry.get());

  // Register local state preferences.
  // TODO(svillar): Ideally we should call chrome::RegisterLocalState()
  // but that pulls in way too many unneeded stuff.
  IOThread::RegisterPrefs(pref_registry.get());
  PrefProxyConfigTrackerImpl::RegisterPrefs(pref_registry.get());
  ssl_config::SSLConfigServiceManager::RegisterPrefs(pref_registry.get());
  GpuModeManager::RegisterPrefs(pref_registry.get());

  pref_change_registrar_.Init(local_state_.get());
  pref_change_registrar_.Add(
    metrics::prefs::kMetricsReportingEnabled,
    base::Bind(&BrowserProcessImpl::ApplyMetricsReportingPolicy,
               base::Unretained(this)));
}

PrefService* BrowserProcessImpl::local_state() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!local_state_)
    CreateLocalState();
  return local_state_.get();
}

bool BrowserProcessImpl::IsShuttingDown() {
  return tearing_down_;
}

void BrowserProcessImpl::PreCreateThreads(
    const base::CommandLine& command_line) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Disable Isolate Extensions by default
  ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      extensions::kExtensionScheme);
#endif

  // initialize local state
  local_state()->UpdateCommandLinePrefStore(
      new ChromeCommandLinePrefStore(&command_line));

  // Must be created before the IOThread.
  // TODO(mmenke): Once IOThread class is no longer needed (not the thread
  // itself), this can be created on first use.
  system_network_context_manager_ =
      base::MakeUnique<SystemNetworkContextManager>();
  io_thread_ = base::MakeUnique<IOThread>(
      local_state(), policy_service(), net_log_.get(),
      extension_event_router_forwarder(),
      system_network_context_manager_.get());

  if (gpu_profile_cache())
    gpu_profile_cache()->Initialize();

  // Create an instance of GpuModeManager to watch gpu mode pref change.
  gpu_mode_manager();
}

void BrowserProcessImpl::PreMainMessageLoopRun() {
  TRACE_EVENT0("startup", "BrowserProcessImpl::PreMainMessageLoopRun");

  ApplyMetricsReportingPolicy();

#if BUILDFLAG(ENABLE_PLUGINS)
  PluginService* plugin_service = PluginService::GetInstance();
  plugin_service->SetFilter(BravePluginServiceFilter::GetInstance());

  // Triggers initialization of the singleton instance on UI thread.
  PluginFinder::GetInstance()->Init();
#endif  // BUILDFLAG(ENABLE_PLUGINS)

#if BUILDFLAG(ENABLE_EXTENSIONS)
  storage_monitor::StorageMonitor::Create();
#endif

// Start the tab manager here so that we give the most amount of time for the
// other services to start up before we start adjusting the oom priority.
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  g_browser_process->GetTabManager()->Start();
#endif
}

resource_coordinator::TabManager* BrowserProcessImpl::GetTabManager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  if (!tab_manager_.get())
    tab_manager_.reset(new resource_coordinator::GuestTabManager());
  return tab_manager_.get();
#else
  return nullptr;
#endif
}

namespace {

// Used at the end of session to block the UI thread for completion of sentinel
// tasks on the set of threads used to persist profile data and local state.
// This is done to ensure that the data has been persisted to disk before
// continuing.
class RundownTaskCounter :
    public base::RefCountedThreadSafe<RundownTaskCounter> {
 public:
  RundownTaskCounter();

  // Posts a rundown task to |task_runner|, can be invoked an arbitrary number
  // of times before calling TimedWait.
  void Post(base::SequencedTaskRunner* task_runner);

  // Waits until the count is zero or |end_time| is reached.
  // This can only be called once per instance. Returns true if a count of zero
  // is reached or false if the |end_time| is reached. It is valid to pass an
  // |end_time| in the past.
  bool TimedWaitUntil(const base::TimeTicks& end_time);

 private:
  friend class base::RefCountedThreadSafe<RundownTaskCounter>;
  ~RundownTaskCounter() {}

  // Decrements the counter and releases the waitable event on transition to
  // zero.
  void Decrement();

  // The count starts at one to defer the possibility of one->zero transitions
  // until TimedWait is called.
  base::AtomicRefCount count_;
  base::WaitableEvent waitable_event_;

  DISALLOW_COPY_AND_ASSIGN(RundownTaskCounter);
};

RundownTaskCounter::RundownTaskCounter()
    : count_(1),
      waitable_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                      base::WaitableEvent::InitialState::NOT_SIGNALED) {}

void RundownTaskCounter::Post(base::SequencedTaskRunner* task_runner) {
  // As the count starts off at one, it should never get to zero unless
  // TimedWait has been called.
  DCHECK(!count_.IsZero());

  count_.Increment();

  // The task must be non-nestable to guarantee that it runs after all tasks
  // currently scheduled on |task_runner| have completed.
  task_runner->PostNonNestableTask(
      FROM_HERE, base::BindOnce(&RundownTaskCounter::Decrement, this));
}

void RundownTaskCounter::Decrement() {
  if (!count_.Decrement())
    waitable_event_.Signal();
}

bool RundownTaskCounter::TimedWaitUntil(const base::TimeTicks& end_time) {
  // Decrement the excess count from the constructor.
  Decrement();

  return waitable_event_.TimedWaitUntil(end_time);
}

}  // namespace

void BrowserProcessImpl::EndSession() {
  // Mark all the profiles as clean.
  ProfileManager* pm = profile_manager();
  std::vector<Profile*> profiles(pm->GetLoadedProfiles());
  scoped_refptr<RundownTaskCounter> rundown_counter(new RundownTaskCounter());
  for (size_t i = 0; i < profiles.size(); ++i) {
    Profile* profile = profiles[i];
    profile->SetExitType(Profile::EXIT_SESSION_ENDED);
    if (profile->GetPrefs()) {
      profile->GetPrefs()->CommitPendingWrite();
      rundown_counter->Post(profile->GetIOTaskRunner().get());
    }
  }

  if (local_state()) {
    local_state()->CommitPendingWrite();
    rundown_counter->Post(local_state_task_runner_.get());
  }

  // http://crbug.com/125207
  base::ThreadRestrictions::ScopedAllowWait allow_wait;

  // We must write that the profile and metrics service shutdown cleanly,
  // otherwise on startup we'll think we crashed. So we block until done and
  // then proceed with normal shutdown.
  //
  // If you change the condition here, be sure to also change
  // ProfileBrowserTests to match.
#if defined(USE_X11) || defined(OS_WIN) || defined(USE_OZONE)
  // Do a best-effort wait on the successful countdown of rundown tasks. Note
  // that if we don't complete "quickly enough", Windows will terminate our
  // process.
  //
  // On Windows, we previously posted a message to FILE and then ran a nested
  // message loop, waiting for that message to be processed until quitting.
  // However, doing so means that other messages will also be processed. In
  // particular, if the GPU process host notices that the GPU has been killed
  // during shutdown, it races exiting the nested loop with the process host
  // blocking the message loop attempting to re-establish a connection to the
  // GPU process synchronously. Because the system may not be allowing
  // processes to launch, this can result in a hang. See
  // http://crbug.com/318527.
  const base::TimeTicks end_time = base::TimeTicks::Now() + kEndSessionTimeout;
  rundown_counter->TimedWaitUntil(end_time);
#else
  NOTIMPLEMENTED();
#endif
}

void BrowserProcessImpl::FlushLocalStateAndReply(base::OnceClosure reply) {
  if (local_state_)
    local_state_->CommitPendingWrite();
  local_state_task_runner_->PostTaskAndReply(
      FROM_HERE, base::Bind(&base::DoNothing), std::move(reply));
}

net_log::ChromeNetLog* BrowserProcessImpl::net_log() {
  return net_log_.get();
}

metrics_services_manager::MetricsServicesManager*
BrowserProcessImpl::GetMetricsServicesManager() {
  NOTIMPLEMENTED();
  return nullptr;
}

metrics::MetricsService* BrowserProcessImpl::metrics_service() {
  return nullptr;
}

net::URLRequestContextGetter* BrowserProcessImpl::system_request_context() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return io_thread()->system_url_request_context_getter();
}

variations::VariationsService* BrowserProcessImpl::variations_service() {
  NOTIMPLEMENTED();
  return nullptr;
}

BrowserProcessPlatformPart* BrowserProcessImpl::platform_part() {
  NOTIMPLEMENTED();
  return nullptr;
}

void BrowserProcessImpl::CreateNotificationUIManager() {
  DCHECK(!notification_ui_manager_);
  notification_ui_manager_.reset(NotificationUIManagerStub::Create());
  created_notification_ui_manager_ = true;
}

NotificationUIManager* BrowserProcessImpl::notification_ui_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!created_notification_ui_manager_)
    CreateNotificationUIManager();
  return notification_ui_manager_.get();
}

NotificationPlatformBridge* BrowserProcessImpl::notification_platform_bridge() {
  NOTIMPLEMENTED();
  return nullptr;
}

IOThread* BrowserProcessImpl::io_thread() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(io_thread_.get());
  return io_thread_.get();
}

SystemNetworkContextManager*
BrowserProcessImpl::system_network_context_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

content::NetworkConnectionTracker*
BrowserProcessImpl::network_connection_tracker() {
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
  return policy_service_stub.Pointer();
}

IconManager* BrowserProcessImpl::icon_manager() {
  NOTIMPLEMENTED();
  return nullptr;
}

GpuModeManager* BrowserProcessImpl::gpu_mode_manager() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!gpu_mode_manager_)
    gpu_mode_manager_ = base::MakeUnique<GpuModeManager>();
  return gpu_mode_manager_.get();
}

GpuProfileCache* BrowserProcessImpl::gpu_profile_cache() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!gpu_profile_cache_)
    gpu_profile_cache_ = GpuProfileCache::Create();
  return gpu_profile_cache_.get();
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

void BrowserProcessImpl::CreateBackgroundPrintingManager() {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  DCHECK(!background_printing_manager_);
  background_printing_manager_ =
      base::MakeUnique<printing::BackgroundPrintingManager>();
#else
  NOTIMPLEMENTED();
#endif
}

IntranetRedirectDetector* BrowserProcessImpl::intranet_redirect_detector() {
  NOTIMPLEMENTED();
  return nullptr;
}

DownloadStatusUpdater* BrowserProcessImpl::download_status_updater() {
  NOTIMPLEMENTED();
  return nullptr;
}

prefs::InProcessPrefServiceFactory* BrowserProcessImpl::pref_service_factory() const {
  NOTIMPLEMENTED();
  return nullptr;
}

DownloadRequestLimiter* BrowserProcessImpl::download_request_limiter() {
  NOTIMPLEMENTED();
  return nullptr;
}

printing::BackgroundPrintingManager*
    BrowserProcessImpl::background_printing_manager() {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!background_printing_manager_)
    CreateBackgroundPrintingManager();
  return background_printing_manager_.get();
#else
  NOTIMPLEMENTED();
  return NULL;
#endif
}

BackgroundModeManager* BrowserProcessImpl::background_mode_manager() {
  NOTIMPLEMENTED();
  return NULL;
}

void BrowserProcessImpl::set_background_mode_manager_for_test(
    std::unique_ptr<BackgroundModeManager> manager) {
  NOTIMPLEMENTED();
}

subresource_filter::ContentRulesetService*
BrowserProcessImpl::subresource_filter_ruleset_service() {
  NOTIMPLEMENTED();
  return nullptr;
}

optimization_guide::OptimizationGuideService*
BrowserProcessImpl::optimization_guide_service() {
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

component_updater::ComponentUpdateService*
BrowserProcessImpl::component_updater() { return nullptr; }
void BrowserProcessImpl::OnKeepAliveStateChanged(bool is_keeping_alive) {}
void BrowserProcessImpl::OnKeepAliveRestartStateChanged(bool can_restart) {}
void BrowserProcessImpl::ResourceDispatcherHostCreated() {}

void BrowserProcessImpl::ApplyMetricsReportingPolicy() {
	GoogleUpdateSettings::CollectStatsConsentTaskRunner()->PostTask(
		FROM_HERE,
		base::BindOnce(
			base::IgnoreResult(&GoogleUpdateSettings::SetCollectStatsConsent),
			ChromeMetricsServiceAccessor::IsMetricsAndCrashReportingEnabled()));
}

#if (defined(OS_WIN) || defined(OS_LINUX)) && !defined(OS_CHROMEOS)
void BrowserProcessImpl::StartAutoupdateTimer() {
  NOTIMPLEMENTED();
}
#endif
