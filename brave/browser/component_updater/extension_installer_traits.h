// Copyright 2016 The Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_EXTENSION_INSTALLER_TRAITS_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_EXTENSION_INSTALLER_TRAITS_H_

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/component_updater/component_installer.h"

namespace base {
class FilePath;
}  // namespace base
using ReadyCallback = base::Callback<void(const base::FilePath&)>;

namespace component_updater {
class ComponentUpdateService;
}

namespace brave {

class ExtensionInstallerTraits :
    public component_updater::ComponentInstallerPolicy {
 public:
  explicit ExtensionInstallerTraits(const std::string &base64_public_key,
                                    const ReadyCallback& ready_callback);
  ~ExtensionInstallerTraits() override {}

 private:
  // The following methods override ComponentInstallerPolicy.
  bool RequiresNetworkEncryption() const override;
  update_client::CrxInstaller::Result OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  bool SupportsGroupPolicyEnabledComponentUpdates() const override;
  std::vector<std::string> GetMimeTypes() const override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  update_client::InstallerAttributes GetInstallerAttributes() const override;

  std::string public_key_;
  std::string base64_public_key_;

  ReadyCallback ready_callback_;
  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallerTraits);
};

// Call once during startup to make the component update service aware of
// the EV whitelist.
void RegisterExtension(component_updater::ComponentUpdateService* cus,
    const std::string& public_key,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_EXTENSION_INSTALLER_TRAITS_H_
