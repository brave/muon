// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browser_process.h"

#include "atom/browser/atom_browser_context.h"
#include "base/command_line.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "chrome/browser/printing/print_job_manager.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/child_process_security_policy.h"
#include "ui/base/idle/idle.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_extensions_browser_client.h"
#include "atom/common/extensions/atom_extensions_client.h"
#include "extensions/common/constants.h"
#include "extensions/common/features/feature_provider.h"
#include "ui/base/resource/resource_bundle.h"
#endif

BrowserProcess* g_browser_process = NULL;

BrowserProcess::BrowserProcess()
    : tearing_down_(false) {
  g_browser_process = this;

  print_job_manager_.reset(new printing::PrintJobManager);

#if defined(OS_MACOSX)
  ui::InitIdleMonitor();
#endif

#if defined(ENABLE_EXTENSIONS)
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      extensions::kExtensionScheme);
  content::ChildProcessSecurityPolicy::GetInstance()->RegisterWebSafeScheme(
      extensions::kExtensionResourceScheme);

  extensions::ExtensionsClient::Set(new extensions::AtomExtensionsClient());
  extensions_browser_client_.reset(
      new extensions::AtomExtensionsBrowserClient());
  extensions::ExtensionsBrowserClient::Set(extensions_browser_client_.get());
  // make sure everything is loaded
  extensions::FeatureProvider::GetAPIFeatures();
  extensions::FeatureProvider::GetPermissionFeatures();
  extensions::FeatureProvider::GetManifestFeatures();
  extensions::FeatureProvider::GetBehaviorFeatures();
#endif
}

BrowserProcess::~BrowserProcess() {
  g_browser_process = NULL;
}

component_updater::ComponentUpdateService*
BrowserProcess::component_updater() {
  if (!component_updater_.get()) {
    auto browser_context = atom::AtomBrowserContext::From("", false);
    scoped_refptr<update_client::Configurator> configurator =
        component_updater::MakeBraveComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess(),
            browser_context->GetRequestContext());
    // Creating the component updater does not do anything, components
    // need to be registered and Start() needs to be called.
    component_updater_.reset(component_updater::ComponentUpdateServiceFactory(
                                 configurator).release());
  }
  return component_updater_.get();
}

void BrowserProcess::StartTearDown() {
  tearing_down_ = true;
  print_job_manager_->Shutdown();
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
