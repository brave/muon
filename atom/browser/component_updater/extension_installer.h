// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_COMPONENT_UPDATER_EV_WHITELIST_COMPONENT_INSTALLER_H_
#define CHROME_BROWSER_COMPONENT_UPDATER_EV_WHITELIST_COMPONENT_INSTALLER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/values.h"
#include "components/component_updater/default_component_installer.h"

namespace base {
class FilePath;
}  // namespace base

namespace component_updater {

class ComponentUpdateService;

class ExtensionInstallerTraits : public ComponentInstallerTraits {
 public:
  ExtensionInstallerTraits(const std::string &public_key);
  ~ExtensionInstallerTraits() override {}

 private:
  // The following methods override ComponentInstallerTraits.
  bool CanAutoUpdate() const override;
  bool RequiresNetworkEncryption() const override;
  bool OnCustomInstall(const base::DictionaryValue& manifest,
                       const base::FilePath& install_dir) override;
  bool VerifyInstallation(const base::DictionaryValue& manifest,
                          const base::FilePath& install_dir) const override;
  void ComponentReady(const base::Version& version,
                      const base::FilePath& install_dir,
                      std::unique_ptr<base::DictionaryValue> manifest) override;
  base::FilePath GetRelativeInstallDir() const override;
  void GetHash(std::vector<uint8_t>* hash) const override;
  std::string GetName() const override;
  std::string GetAp() const override;

  std::string public_key_;
  DISALLOW_COPY_AND_ASSIGN(ExtensionInstallerTraits);
};

// Call once during startup to make the component update service aware of
// the EV whitelist.
void RegisterExtension(ComponentUpdateService* cus,
    const std::string& public_key);

}  // namespace component_updater

#endif  // CHROME_BROWSER_COMPONENT_UPDATER_EV_WHITELIST_COMPONENT_INSTALLER_H_
