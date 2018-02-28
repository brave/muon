// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "muon/browser/muon_browser_process_impl.h"

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "brave/browser/component_updater/brave_component_updater_configurator.h"
#include "chrome/browser/chrome_device_client.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/safe_browsing/safe_browsing_service.h"
#include "components/component_updater/component_updater_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/resource_dispatcher_host.h"

MuonBrowserProcessImpl::MuonBrowserProcessImpl(
      base::SequencedTaskRunner* local_state_task_runner) :
    BrowserProcessImpl(local_state_task_runner) {
  g_browser_process = this;

  device_client_.reset(new ChromeDeviceClient);
}

MuonBrowserProcessImpl::~MuonBrowserProcessImpl() {
  g_browser_process = NULL;
}

component_updater::ComponentUpdateService*
MuonBrowserProcessImpl::component_updater(
    std::unique_ptr<component_updater::ComponentUpdateService> &component_updater,
    bool use_brave_server) {
  if (!component_updater.get()) {
    if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI))
      return NULL;
    scoped_refptr<update_client::Configurator> configurator =
        component_updater::MakeBraveComponentUpdaterConfigurator(
            base::CommandLine::ForCurrentProcess(),
            io_thread()->system_url_request_context_getter(),
            use_brave_server);
    // Creating the component updater does not do anything, components
    // need to be registered and Start() needs to be called.
    component_updater.reset(component_updater::ComponentUpdateServiceFactory(
                                 configurator).release());
  }
  return component_updater.get();
}

component_updater::ComponentUpdateService*
MuonBrowserProcessImpl::brave_component_updater() {
  return component_updater(brave_component_updater_, true);
}

component_updater::ComponentUpdateService*
MuonBrowserProcessImpl::component_updater() {
  return component_updater(component_updater_, false);
}

void MuonBrowserProcessImpl::ResourceDispatcherHostCreated() {
  resource_dispatcher_host_delegate_.reset(
      new atom::AtomResourceDispatcherHostDelegate);
  content::ResourceDispatcherHost::Get()->SetDelegate(
      resource_dispatcher_host_delegate_.get());
}
