// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MUON_BROWSER_MUON_BROWSER_PROCESS_IMPL_H_
#define MUON_BROWSER_MUON_BROWSER_PROCESS_IMPL_H_

#include "build/build_config.h"
#include "chrome/browser/browser_process_impl.h"

class ChromeDeviceClient;

namespace atom {
namespace api {
class App;
}
}  // namespace atom

namespace atom {
class AtomResourceDispatcherHostDelegate;
}

// Real implementation of BrowserProcess that creates and returns the services.
class MuonBrowserProcessImpl : public BrowserProcessImpl {
 public:
  MuonBrowserProcessImpl(base::SequencedTaskRunner* local_state_task_runner);
  ~MuonBrowserProcessImpl() override;

  void set_app(atom::api::App* app) { app_ = app; }
  atom::api::App* app() { return app_; }

  component_updater::ComponentUpdateService* brave_component_updater();
  component_updater::ComponentUpdateService* component_updater() override;

  void ResourceDispatcherHostCreated() override;
 private:
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

  std::unique_ptr<ChromeDeviceClient> device_client_;

  DISALLOW_COPY_AND_ASSIGN(MuonBrowserProcessImpl);
};

#endif  // MUON_BROWSER_MUON_BROWSER_PROCESS_IMPL_H_
