// Copyright 2016 Brave authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/component_updater/extension_installer_traits.h"

#include <string.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/version.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/sha2.h"
#include "extensions/common/constants.h"
#include "extensions/common/manifest_constants.h"

using component_updater::ComponentUpdateService;

namespace {
  bool RewriteManifestFile(
    const base::FilePath& extension_root,
    const base::DictionaryValue& manifest,
    const std::string &public_key) {

  // Add the public key
  DCHECK(!public_key.empty());

  std::unique_ptr<base::DictionaryValue> final_manifest(manifest.DeepCopy());
  final_manifest->SetString(extensions::manifest_keys::kPublicKey, public_key);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*final_manifest)) {
    return false;
  }

  base::FilePath manifest_path =
    extension_root.Append(extensions::kManifestFilename);
  int size = base::checked_cast<int>(manifest_json.size());
  if (base::WriteFile(manifest_path, manifest_json.data(), size) != size) {
    return false;
  }
  return true;
}

}  // namespace

namespace brave {

ExtensionInstallerTraits::ExtensionInstallerTraits(
    const std::string &base64_public_key,
    const ReadyCallback& ready_callback) :
    base64_public_key_(base64_public_key),
    ready_callback_(ready_callback) {
  base::Base64Decode(base64_public_key, &public_key_);
}

bool ExtensionInstallerTraits::RequiresNetworkEncryption() const {
  return false;
}

update_client::CrxInstaller::Result ExtensionInstallerTraits::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

void ExtensionInstallerTraits::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
  std::unique_ptr<base::DictionaryValue> manifest) {
  ready_callback_.Run(install_dir);
}

bool
ExtensionInstallerTraits::SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

std::vector<std::string> ExtensionInstallerTraits::GetMimeTypes() const {
  return std::vector<std::string>();
}

// Called during startup and installation before ComponentReady().
bool ExtensionInstallerTraits::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {

  // The manifest file will generate a random ID if we don't provide one.
  // We want to write one with the actual extensions public key so we get
  // the same extensionID which is generated from the public key.
  if (!RewriteManifestFile(install_dir, manifest, base64_public_key_)) {
    return false;
  }
  return base::PathExists(
      install_dir.Append(FILE_PATH_LITERAL("manifest.json")));
}

base::FilePath
ExtensionInstallerTraits::GetRelativeInstallDir() const {
  // Get the extension ID from the public key
  std::string extension_id = crx_file::id_util::GenerateId(public_key_);
  return base::FilePath(
      // Convert to wstring or string depending on OS
      base::FilePath::StringType(extension_id.begin(), extension_id.end()));
}

void ExtensionInstallerTraits::GetHash(
    std::vector<uint8_t>* hash) const {
  const std::string public_key_sha256 = crypto::SHA256HashString(public_key_);
  hash->assign(public_key_sha256.begin(), public_key_sha256.end());
}

std::string ExtensionInstallerTraits::GetName() const {
  return std::string();
}

update_client::InstallerAttributes
ExtensionInstallerTraits::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

void RegisterExtension(
    ComponentUpdateService* cus,
    const std::string& public_key,
    const base::Closure& registered_callback,
    const ReadyCallback& ready_callback) {
  std::unique_ptr<component_updater::ComponentInstallerPolicy> traits(
      new ExtensionInstallerTraits(public_key, ready_callback));

  // |cus| will take ownership of |installer| during installer->Register(cus).
  component_updater::ComponentInstaller* installer =
      new component_updater::ComponentInstaller(std::move(traits));
  installer->Register(cus, registered_callback);
}

}  // namespace brave
