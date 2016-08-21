// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.


#ifndef BRAVE_BROWSER_CRX_INSTALLER_H_
#define BRAVE_BROWSER_CRX_INSTALLER_H_

#include "extensions/browser/sandboxed_unpacker.h"
#include "extensions/browser/install/crx_install_error.h"
#include "base/sequenced_task_runner.h"

namespace brave {

class CrxInstaller : public extensions::SandboxedUnpackerClient {
 public:
  CrxInstaller();

  void InstallCrx(const base::FilePath &path);
  void UpdateFromCrx(const base::FilePath &path,
                     const std::string& extension_id);

  // SandboxedUnpackerClient
  void OnUnpackFailure(const extensions::CrxInstallError& error) override;
  void OnUnpackSuccess(const base::FilePath& temp_dir,
                       const base::FilePath& extension_dir,
                       const base::DictionaryValue* original_manifest,
                       const extensions::Extension* extension,
                       const SkBitmap& install_icon) override;
  void NotifyCrxInstallComplete(bool success);
  void NotifyCrxInstallError(const extensions::CrxInstallError &error);
 private:

  base::SequencedTaskRunner* GetFileTaskRunner();

  // Sequenced task runner where file I/O operations will be performed.
  scoped_refptr<base::SequencedTaskRunner> installer_task_runner_;
  scoped_refptr<const extensions::Extension> extension_;
  int creation_flags_;
  extensions::Manifest::Location install_source_;
  base::FilePath install_directory_;
};

}  // namespace

#endif  // BRAVE_BROWSER_CRX_INSTALLER_H_
