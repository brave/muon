// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_CONTENT_CLIENT_H_
#define ATOM_APP_ATOM_CONTENT_CLIENT_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "brightray/common/content_client.h"
#include "chrome/common/origin_trials/chrome_origin_trial_policy.h"

namespace atom {

class AtomContentClient : public brightray::ContentClient {
 public:
  AtomContentClient();
  virtual ~AtomContentClient();

 protected:
  // content::ContentClient:
  void SetActiveURL(const GURL& url, std::string top_origin) override;
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  void SetGpuInfo(const gpu::GPUInfo& gpu_info);
  void AddAdditionalSchemes(Schemes* schemes) override;
  void AddPepperPlugins(
      std::vector<content::PepperPluginInfo>* plugins) override;
  bool AllowScriptExtensionForServiceWorker(const GURL& script_url) override;
  content::OriginTrialPolicy* GetOriginTrialPolicy() override;

  void AddContentDecryptionModules(
      std::vector<content::CdmInfo>* cdms,
      std::vector<media::CdmHostFilePath>* cdm_host_file_paths) override;

 private:
  std::unique_ptr<ChromeOriginTrialPolicy> origin_trial_policy_;
  DISALLOW_COPY_AND_ASSIGN(AtomContentClient);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_CONTENT_CLIENT_H_
