// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// When each service is created, we set a flag indicating this. At this point,
// the service initialization could fail or succeed. This allows us to remember
// if we tried to create a service, and not try creating it over and over if
// the creation failed.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_
#define CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/debug/stack_trace.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "extensions/features/features.h"
#include "net/log/net_log.h"

class TabManager;

namespace atom {

class AtomResourceDispatcherHostDelegate;

namespace api {
class App;
}

}  // namespace atom

namespace ukm {
class UkmRecorder;
}

namespace base {
class SequencedTaskRunner;
}

namespace chrome {
class ChromeNetLog;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace extensions {
class EventRouterForwarder;
class ExtensionsBrowserClient;
}
#endif

// Real implementation of BrowserProcess that creates and returns the services.
class BrowserProcessImpl : public BrowserProcess {
 public:
  // |local_state_task_runner| must be a shutdown-blocking task runner.
  BrowserProcessImpl(base::SequencedTaskRunner* local_state_task_runner);
  ~BrowserProcessImpl() override;

  void PreCreateThreads();
  void PreMainMessageLoopRun();
  const std::string& GetApplicationLocale() override;

  printing::PrintJobManager* print_job_manager() override;

  bool IsShuttingDown();
  void StartTearDown();

  void set_app(atom::api::App* app) { app_ = app; }
  atom::api::App* app() { return app_; }

  void ResourceDispatcherHostCreated() override;
  metrics::MetricsService* metrics_service() override { return NULL; };
  PrefService* local_state() override;
  ProfileManager* profile_manager() override;
  ukm::UkmRecorder* ukm_recorder() override;
  rappor::RapporServiceImpl* rappor_service() override;
  component_updater::ComponentUpdateService* brave_component_updater();
  component_updater::ComponentUpdateService* component_updater() override;
  extensions::EventRouterForwarder*
      extension_event_router_forwarder() override;
  resource_coordinator::TabManager* GetTabManager() override;
  StatusTray* status_tray() override;
  message_center::MessageCenter* message_center() override;
  net_log::ChromeNetLog* net_log() override;

  // NOTIMPLEMENTED
  void EndSession() override;
  metrics_services_manager::MetricsServicesManager*
  GetMetricsServicesManager() override;
  net::URLRequestContextGetter* system_request_context() override;
  variations::VariationsService* variations_service() override;
  BrowserProcessPlatformPart* platform_part() override;
  NotificationUIManager* notification_ui_manager() override;
  NotificationPlatformBridge* notification_platform_bridge() override;
  IOThread* io_thread() override;
  WatchDogThread* watchdog_thread() override;
  policy::BrowserPolicyConnector* browser_policy_connector() override;
  policy::PolicyService* policy_service() override;
  IconManager* icon_manager() override;
  GpuModeManager* gpu_mode_manager() override;
  GpuProfileCache* gpu_profile_cache() override;
  void CreateDevToolsHttpProtocolHandler(const std::string& ip,
                                         uint16_t port) override;
  void CreateDevToolsAutoOpener() override;
  printing::PrintPreviewDialogController* print_preview_dialog_controller()
      override;
  printing::BackgroundPrintingManager* background_printing_manager() override;
  IntranetRedirectDetector* intranet_redirect_detector() override;
  void SetApplicationLocale(const std::string& locale) override;
  DownloadStatusUpdater* download_status_updater() override;
  DownloadRequestLimiter* download_request_limiter() override;
  BackgroundModeManager* background_mode_manager() override;
  void set_background_mode_manager_for_test(
      std::unique_ptr<BackgroundModeManager> manager) override;
  safe_browsing::SafeBrowsingService* safe_browsing_service() override;
  safe_browsing::ClientSideDetectionService* safe_browsing_detection_service()
      override;
  subresource_filter::ContentRulesetService* subresource_filter_ruleset_service()
      override;
  CRLSetFetcher* crl_set_fetcher() override;
  component_updater::PnaclComponentInstaller* pnacl_component_installer()
      override;
  component_updater::SupervisedUserWhitelistInstaller*
  supervised_user_whitelist_installer() override;
  MediaFileSystemRegistry* media_file_system_registry() override;
  bool created_local_state() const override;
#if BUILDFLAG(ENABLE_WEBRTC)
  WebRtcLogUploader* webrtc_log_uploader() override;
#endif
  network_time::NetworkTimeTracker* network_time_tracker() override;
  gcm::GCMDriver* gcm_driver() override;
  shell_integration::DefaultWebClientState CachedDefaultWebClientState()
      override;
  physical_web::PhysicalWebDataSource* GetPhysicalWebDataSource() override;
#if (defined(OS_WIN) || defined(OS_LINUX)) && !defined(OS_CHROMEOS)
  void StartAutoupdateTimer() override;
#endif
  prefs::InProcessPrefServiceFactory* pref_service_factory() const override;

 private:
  base::ThreadChecker thread_checker_;

  void CreateLocalState();
  void CreateProfileManager();
  void CreateStatusTray();


  const scoped_refptr<base::SequencedTaskRunner> local_state_task_runner_;
  bool tearing_down_;
  bool created_profile_manager_;
  bool created_local_state_;

  std::unique_ptr<printing::PrintJobManager> print_job_manager_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  scoped_refptr<extensions::EventRouterForwarder>
      extension_event_router_forwarder_;
  std::unique_ptr<extensions::ExtensionsBrowserClient> extensions_browser_client_;
#endif

  std::string locale_;

  std::unique_ptr<ProfileManager> profile_manager_;

  atom::api::App* app_;  // not owned

  std::unique_ptr<atom::AtomResourceDispatcherHostDelegate>
      resource_dispatcher_host_delegate_;

  std::unique_ptr<component_updater::ComponentUpdateService>
      brave_component_updater_;
  std::unique_ptr<component_updater::ComponentUpdateService>
      component_updater_;
  component_updater::ComponentUpdateService* component_updater(
      std::unique_ptr<component_updater::ComponentUpdateService> &,
      bool use_brave_server);

#if defined(OS_WIN) || defined(OS_MACOSX) || defined(OS_LINUX)
  // Any change to this #ifdef must be reflected as well in
  // chrome/browser/resource_coordinator/tab_manager_browsertest.cc
  std::unique_ptr<resource_coordinator::TabManager> tab_manager_;
#endif

  std::unique_ptr<PrefService> local_state_;

  DISALLOW_COPY_AND_ASSIGN(BrowserProcessImpl);
};

#endif  // CHROME_BROWSER_BROWSER_PROCESS_IMPL_H_
