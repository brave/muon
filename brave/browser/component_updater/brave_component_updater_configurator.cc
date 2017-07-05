// Copyright 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/component_updater/brave_component_updater_configurator.h"

#include <stdint.h>

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/strings/sys_string_conversions.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/version.h"
#if defined(OS_WIN)
#include "base/win/win_util.h"
#endif
#include "chrome/browser/browser_process.h"
#include "components/component_updater/configurator_impl.h"
#include "components/prefs/pref_service.h"
#include "components/update_client/component_patcher_operation.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context_getter.h"


namespace component_updater {

namespace {

class BraveConfigurator : public update_client::Configurator {
 public:
  BraveConfigurator(const base::CommandLine* cmdline,
                    net::URLRequestContextGetter* url_request_getter,
                    bool use_brave_server);

  // update_client::Configurator overrides.
  int InitialDelay() const override;
  int NextCheckDelay() const override;
  int OnDemandDelay() const override;
  int UpdateDelay() const override;
  std::vector<GURL> UpdateUrl() const override;
  std::vector<GURL> PingUrl() const override;
  std::string GetProdId() const override;
  base::Version GetBrowserVersion() const override;
  std::string GetChannel() const override;
  std::string GetBrand() const override;
  std::string GetLang() const override;
  std::string GetOSLongName() const override;
  std::string ExtraRequestParams() const override;
  std::string GetDownloadPreference() const override;
  net::URLRequestContextGetter* RequestContext() const override;
  scoped_refptr<update_client::OutOfProcessPatcher> CreateOutOfProcessPatcher()
      const override;
  bool EnabledDeltas() const override;
  bool EnabledComponentUpdates() const override;
  bool EnabledBackgroundDownloader() const override;
  bool EnabledCupSigning() const override;
  scoped_refptr<base::SequencedTaskRunner> GetSequencedTaskRunner()
      const override;
  PrefService* GetPrefService() const override;
  bool IsPerUserInstall() const override;
  std::vector<uint8_t> GetRunActionKeyHash() const override;

 private:
  friend class base::RefCountedThreadSafe<BraveConfigurator>;

  ConfiguratorImpl configurator_impl_;
  bool use_brave_server_;

  ~BraveConfigurator() override {}
};

// Allows the component updater to use non-encrypted communication with the
// update backend. The security of the update checks is enforced using
// a custom message signing protocol and it does not depend on using HTTPS.
BraveConfigurator::BraveConfigurator(
    const base::CommandLine* cmdline,
    net::URLRequestContextGetter* url_request_getter,
    bool use_brave_server)
    : configurator_impl_(cmdline, url_request_getter, false),
      use_brave_server_(use_brave_server) {}

int BraveConfigurator::InitialDelay() const {
  return configurator_impl_.InitialDelay();
}

int BraveConfigurator::NextCheckDelay() const {
  return configurator_impl_.NextCheckDelay();
}

int BraveConfigurator::OnDemandDelay() const {
  return configurator_impl_.OnDemandDelay();
}

int BraveConfigurator::UpdateDelay() const {
  return configurator_impl_.UpdateDelay();
}

std::vector<GURL> BraveConfigurator::UpdateUrl() const {
  if (use_brave_server_) {
    // For localhost of vault-updater
    // return std::vector<GURL> {GURL("http://localhost:8192/extensions")};
    return std::vector<GURL>
        {GURL("https://laptop-updates.brave.com/extensions")};
  }

  // For Chrome's component store
  return configurator_impl_.UpdateUrl();
}

std::vector<GURL> BraveConfigurator::PingUrl() const {
  return UpdateUrl();
}

std::string BraveConfigurator::GetProdId() const {
  return std::string();
}

base::Version BraveConfigurator::GetBrowserVersion() const {
  return configurator_impl_.GetBrowserVersion();
}

std::string BraveConfigurator::GetChannel() const {
  return std::string("stable");
}

std::string BraveConfigurator::GetBrand() const {
  return std::string();
}

std::string BraveConfigurator::GetLang() const {
  return std::string();
}

std::string BraveConfigurator::GetOSLongName() const {
  return configurator_impl_.GetOSLongName();
}

std::string BraveConfigurator::ExtraRequestParams() const {
  return configurator_impl_.ExtraRequestParams();
}

std::string BraveConfigurator::GetDownloadPreference() const {
  return std::string();
}

net::URLRequestContextGetter* BraveConfigurator::RequestContext() const {
  return configurator_impl_.RequestContext();
}

scoped_refptr<update_client::OutOfProcessPatcher>
BraveConfigurator::CreateOutOfProcessPatcher() const {
  return nullptr;
}

bool BraveConfigurator::EnabledComponentUpdates() const {
  return configurator_impl_.EnabledComponentUpdates();
}

bool BraveConfigurator::EnabledDeltas() const {
  // TODO(bbondy): Re-enable
  // return configurator_impl_.DeltasEnabled();
  return false;
}

bool BraveConfigurator::EnabledBackgroundDownloader() const {
  return configurator_impl_.EnabledBackgroundDownloader();
}

bool BraveConfigurator::EnabledCupSigning() const {
  if (use_brave_server_) {
    return false;
  }
  return configurator_impl_.EnabledCupSigning();
}

// Returns a task runner to run blocking tasks. The task runner continues to run
// after the browser shuts down, until the OS terminates the process. This
// imposes certain requirements for the code using the task runner, such as
// not accessing any global browser state while the code is running.
scoped_refptr<base::SequencedTaskRunner>
BraveConfigurator::GetSequencedTaskRunner() const {
  return content::BrowserThread::GetBlockingPool()
      ->GetSequencedTaskRunnerWithShutdownBehavior(
          base::SequencedWorkerPool::GetSequenceToken(),
          base::SequencedWorkerPool::CONTINUE_ON_SHUTDOWN);
}

PrefService* BraveConfigurator::GetPrefService() const {
  return nullptr;
}

bool BraveConfigurator::IsPerUserInstall() const {
  return false;
}

std::vector<uint8_t> BraveConfigurator::GetRunActionKeyHash() const {
  return configurator_impl_.GetRunActionKeyHash();
}

}  // namespace

scoped_refptr<update_client::Configurator>
MakeBraveComponentUpdaterConfigurator(
    const base::CommandLine* cmdline,
    net::URLRequestContextGetter* context_getter,
    bool use_brave_server) {
  return new BraveConfigurator(cmdline, context_getter, use_brave_server);
}

}  // namespace component_updater
