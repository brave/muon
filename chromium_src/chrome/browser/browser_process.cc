// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/atom_browser_context.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/threading/thread.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "ppapi/features/features.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/message_center/message_center.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "chrome/browser/extensions/event_router_forwarder.h"
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

using content::PluginService;

BrowserProcess* g_browser_process = NULL;

BrowserProcess::BrowserProcess()
    : tearing_down_(false),
      created_profile_manager_(false) {
  g_browser_process = this;

  print_job_manager_.reset(new printing::PrintJobManager);

#if defined(OS_MACOSX)
  ui::InitIdleMonitor();
#endif

#if BUILDFLAG(ENABLE_EXTENSIONS)
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      extensions::kExtensionScheme);

  extension_event_router_forwarder_ = new extensions::EventRouterForwarder;

  extensions::ExtensionsClient::Set(
      extensions::ChromeExtensionsClient::GetInstance());
  extensions_browser_client_.reset(
      new extensions::AtomExtensionsBrowserClient());
  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());
#endif

  message_center::MessageCenter::Initialize();
}

BrowserProcess::~BrowserProcess() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionsBrowserClient::Set(nullptr);
#endif
  g_browser_process = NULL;
}

void BrowserProcess::CreateProfileManager() {
  DCHECK(!created_profile_manager_ && !profile_manager_);
  created_profile_manager_ = true;

  base::FilePath user_data_dir;
  PathService::Get(brightray::DIR_USER_DATA, &user_data_dir);
  profile_manager_.reset(new ProfileManager(user_data_dir));
}

ProfileManager* BrowserProcess::profile_manager() {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (!created_profile_manager_)
    CreateProfileManager();
  return profile_manager_.get();
}

rappor::RapporService* BrowserProcess::rappor_service() {
  return nullptr;
}

component_updater::ComponentUpdateService*
BrowserProcess::component_updater(
    std::unique_ptr<component_updater::ComponentUpdateService> &component_updater,
    bool use_brave_server) {
  if (!component_updater.get()) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
      return NULL;
    auto browser_context = atom::AtomBrowserContext::From("", false);
    scoped_refptr<update_client::Configurator> configurator =
        component_updater::MakeBraveComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess(),
            browser_context->GetRequestContext(),
            use_brave_server);
    // Creating the component updater does not do anything, components
    // need to be registered and Start() needs to be called.
    component_updater.reset(component_updater::ComponentUpdateServiceFactory(
                                 configurator).release());
  }
  return component_updater.get();
}

component_updater::ComponentUpdateService*
BrowserProcess::brave_component_updater() {
  return component_updater(brave_component_updater_, true);
}

component_updater::ComponentUpdateService*
BrowserProcess::component_updater() {
  return component_updater(component_updater_, false);
}

extensions::EventRouterForwarder*
BrowserProcess::extension_event_router_forwarder() {
  return extension_event_router_forwarder_.get();
}

void BrowserProcess::StartTearDown() {
  tearing_down_ = true;
  print_job_manager_->Shutdown();

  profile_manager_.reset();

  message_center::MessageCenter::Shutdown();
}

std::string BrowserProcess::GetApplicationLocale() {
  return l10n_util::GetApplicationLocale("");
}

printing::PrintJobManager* BrowserProcess::print_job_manager() {
  return print_job_manager_.get();
}

bool BrowserProcess::IsShuttingDown() {
  return tearing_down_;
}

void BrowserProcess::PreCreateThreads() {
}

void BrowserProcess::PreMainMessageLoopRun() {
#if BUILDFLAG(ENABLE_PLUGINS)
  PluginService* plugin_service = PluginService::GetInstance();
  plugin_service->SetFilter(BravePluginServiceFilter::GetInstance());

  // Triggers initialization of the singleton instance on UI thread.
  PluginFinder::GetInstance()->Init();

#endif  // BUILDFLAG(ENABLE_PLUGINS)
}
