// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This interface is for managing the global services of the application. Each
// service is lazily created when requested the first time. The service getters
// will return NULL if the service is not available, so callers must check for
// this condition.

#ifndef CHROME_BROWSER_BROWSER_PROCESS_H_
#define CHROME_BROWSER_BROWSER_PROCESS_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"
#include "extensions/features/features.h"
#include "net/log/net_log.h"

namespace atom {
namespace api {
class App;
}
}

namespace component_updater {
class ComponentUpdateService;
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
namespace extensions {
class EventRouterForwarder;
class ExtensionsBrowserClient;
}
#endif

namespace metrics {
class MetricsService;
}
namespace printing {
class PrintJobManager;
}

namespace rappor {
class RapporService;
}

class IOThread;
class PrefService;
class ProfileManager;
// NOT THREAD SAFE, call only from the main thread.
// These functions shouldn't return NULL unless otherwise noted.
class BrowserProcess {
 public:
  BrowserProcess();
  ~BrowserProcess();

  void PreCreateThreads();
  void PreMainMessageLoopRun();
  std::string GetApplicationLocale();

  printing::PrintJobManager* print_job_manager();

  bool IsShuttingDown();
  void StartTearDown();

  void set_app(atom::api::App* app) { app_ = app; }
  atom::api::App* app() { return app_; }

  metrics::MetricsService* metrics_service() { return NULL; };
  PrefService* local_state();
  ProfileManager* profile_manager();
  rappor::RapporService* rappor_service();
  component_updater::ComponentUpdateService* brave_component_updater();
  component_updater::ComponentUpdateService* component_updater();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::EventRouterForwarder* extension_event_router_forwarder();
#endif

 private:
  base::ThreadChecker thread_checker_;

  void CreateProfileManager();

  std::unique_ptr<printing::PrintJobManager> print_job_manager_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  scoped_refptr<extensions::EventRouterForwarder>
      extension_event_router_forwarder_;
  std::unique_ptr<extensions::ExtensionsBrowserClient> extensions_browser_client_;
#endif
  bool tearing_down_;

  bool created_profile_manager_;
  std::unique_ptr<ProfileManager> profile_manager_;

  atom::api::App* app_;  // not owned

  net::NetLog net_log_;
  std::unique_ptr<component_updater::ComponentUpdateService>
      brave_component_updater_;
  std::unique_ptr<component_updater::ComponentUpdateService>
      component_updater_;
  component_updater::ComponentUpdateService* component_updater(
      std::unique_ptr<component_updater::ComponentUpdateService> &,
      bool use_brave_server);
  DISALLOW_COPY_AND_ASSIGN(BrowserProcess);
};

extern BrowserProcess* g_browser_process;

#endif  // CHROME_BROWSER_BROWSER_PROCESS_H_
