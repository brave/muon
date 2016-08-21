// Copyright (c) 2016 Brave.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/browser/crx_installer.h"

#include "extensions/common/extension.h"
#include "brave/browser/brave_extensions.h"
#include "content/public/browser/notification_service.h"
#include "extensions/browser/crx_file_info.h"
#include "extensions/browser/notification_types.h"
#include "extensions/common/file_util.h"
#include "atom/app/atom_main_delegate.h"
#include "browser/brightray_paths.h"
#include "base/path_service.h"
#include "atom/browser/api/atom_api_extension.h"
#include "atom/browser/extensions/atom_notification_types.h"

using extensions::SandboxedUnpacker;
using extensions::Extension;
using extensions::Manifest;
using extensions::CRXFileInfo;
using content::BrowserThread;

namespace brave {

CrxInstaller::CrxInstaller() :
    creation_flags_(Extension::NO_FLAGS),
    install_source_(Manifest::INTERNAL)
{
}

scoped_refptr<SandboxedUnpacker> unpacker;
void CrxInstaller::InstallCrx(const base::FilePath &source_file) {
  UpdateFromCrx(source_file, "");
}

void CrxInstaller::UpdateFromCrx(const base::FilePath &source_file,
                                 const std::string& extension_id) {
  PathService::Get(brightray::DIR_USER_DATA, &install_directory_);
  install_directory_ =
    install_directory_.Append(FILE_PATH_LITERAL("Extensions"));
  creation_flags_ |= Extension::ALLOW_FILE_ACCESS;
  creation_flags_ |= Extension::FROM_WEBSTORE;
  base::SequencedTaskRunner* task_runner = GetFileTaskRunner();

  if (extension_id.length() > 0) {
    const extensions::Extension* extension =
      BraveExtensions::Get()->GetByID(extension_id);
    if (!extension) {
      LOG(INFO) << "--------No ID so not doing update for installCrx";
      return;
    }
    install_source_ = extension->location();
    LOG(INFO) << "--------Doing update for installCrx:" << install_source_;
      //"aomjjhallfgjeglblehebfpbcfeobpgk"
  } else {
    LOG(INFO) << "--------New install for installCrx";
  }

  //scoped_refptr<SandboxedUnpacker> unpacker(
  unpacker = (
      new SandboxedUnpacker(
        install_source_, creation_flags_, install_directory_,
        task_runner, this));

  if (!task_runner->PostTask(
          FROM_HERE, base::Bind(&SandboxedUnpacker::StartWithCrx,
                                unpacker.get(),
                                CRXFileInfo(extension_id, source_file)))) {
    NOTREACHED();
  }
}

void CrxInstaller::OnUnpackFailure(const extensions::CrxInstallError& error) {
  LOG(ERROR) << "------Install CRX failure:"
    << "\ntype:" << error.type()
    << "\nmessage:" << error.message();

  if (!BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CrxInstaller::NotifyCrxInstallError, this, error))) {
    NOTREACHED();
  }
}

void CrxInstaller::OnUnpackSuccess(const base::FilePath& temp_dir,
    const base::FilePath& extension_dir,
    const base::DictionaryValue* original_manifest,
    const Extension* extension,
    const SkBitmap& install_icon) {

  std::string extension_id = extension->id();
  base::FilePath version_dir = extensions::file_util::InstallExtension(
      extension->path(),
      extension_id,
      extension->VersionString(),
      install_directory_);

  LOG(INFO) << "------Install CRX success" << version_dir.value();


  // The extension moved so load a new one, this is the same as Chrome does
  std::string error;
  extension_ = extensions::file_util::LoadExtension(
      version_dir,
      install_source_,
      // Note: modified by UpdateCreationFlagsAndCompleteInstall.
      creation_flags_,
      &error).get();
  if (!BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&CrxInstaller::NotifyCrxInstallComplete, this, true))) {
    NOTREACHED();
  }
}

void CrxInstaller::NotifyCrxInstallComplete(bool success) {
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_CRX_INSTALLER_DONE,
      content::Source<CrxInstaller>(this),
      content::Details<const extensions::Extension>(success ? extension_.get() : NULL));

    content::NotificationService::current()->Notify(
        atom::NOTIFICATION_ENABLE_USER_EXTENSION_REQUEST,
        content::Source<CrxInstaller>(this),
        content::Details<const extensions::Extension>(extension_.get()));
}

void CrxInstaller::NotifyCrxInstallError(const extensions::CrxInstallError &error) {
  content::NotificationService::current()->Notify(
      extensions::NOTIFICATION_EXTENSION_INSTALL_ERROR,
      content::Source<CrxInstaller>(this),
      content::Details<const extensions::CrxInstallError>(&error));
  NotifyCrxInstallComplete(false);
}

base::SequencedTaskRunner* CrxInstaller::GetFileTaskRunner() {
  if (installer_task_runner_.get())
    return installer_task_runner_.get();

  // We should be able to interrupt any part of extension install process during
  // shutdown. SKIP_ON_SHUTDOWN ensures that not started extension install tasks
  // will be ignored/deleted while we will block on started tasks.
  installer_task_runner_ = BrowserThread::GetBlockingPool()->
      GetSequencedTaskRunnerWithShutdownBehavior(
        BrowserThread::GetBlockingPool()->GetSequenceToken(),
        base::SequencedWorkerPool::SKIP_ON_SHUTDOWN);
  return installer_task_runner_.get();
}

}  // namespace brave
