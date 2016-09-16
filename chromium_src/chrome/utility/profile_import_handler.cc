// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/utility/profile_import_handler.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/common/importer/profile_import_process_messages.h"
#include "chrome/utility/importer/external_process_importer_bridge.h"
#include "chrome/utility/importer/importer.h"
#include "chrome/utility/importer/importer_creator.h"
#include "content/public/utility/utility_thread.h"

namespace {

bool Send(IPC::Message* message) {
  return content::UtilityThread::Get()->Send(message);
}

}  // namespace

ProfileImportHandler::ProfileImportHandler() : items_to_import_(0) {}

ProfileImportHandler::~ProfileImportHandler() {}

bool ProfileImportHandler::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(ProfileImportHandler, message)
    IPC_MESSAGE_HANDLER(ProfileImportProcessMsg_StartImport, OnImportStart)
    IPC_MESSAGE_HANDLER(ProfileImportProcessMsg_CancelImport, OnImportCancel)
    IPC_MESSAGE_HANDLER(ProfileImportProcessMsg_ReportImportItemFinished,
                        OnImportItemFinished)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void ProfileImportHandler::OnImportStart(
    const importer::SourceProfile& source_profile,
    uint16_t items,
    const base::DictionaryValue& localized_strings) {
  content::UtilityThread::Get()->EnsureBlinkInitialized();
  bridge_ = new ExternalProcessImporterBridge(
      localized_strings,
      content::UtilityThread::Get(),
      base::ThreadTaskRunnerHandle::Get().get());
  importer_ = importer::CreateImporterByType(source_profile.importer_type);
  if (!importer_.get()) {
    Send(new ProfileImportProcessHostMsg_Import_Finished(
        false, "Importer could not be created."));
    return;
  }

  items_to_import_ = items;

  // Create worker thread in which importer runs.
  import_thread_.reset(new base::Thread("import_thread"));
#if defined(OS_WIN)
  import_thread_->init_com_with_mta(false);
#endif
  if (!import_thread_->Start()) {
    NOTREACHED();
    ImporterCleanup();
  }
  import_thread_->task_runner()->PostTask(
      FROM_HERE, base::Bind(&Importer::StartImport, importer_.get(),
                            source_profile, items, base::RetainedRef(bridge_)));
}

void ProfileImportHandler::OnImportCancel() {
  ImporterCleanup();
}

void ProfileImportHandler::OnImportItemFinished(uint16_t item) {
  items_to_import_ ^= item;  // Remove finished item from mask.
  // If we've finished with all items, notify the browser process.
  if (items_to_import_ == 0) {
    Send(new ProfileImportProcessHostMsg_Import_Finished(true, std::string()));
    ImporterCleanup();
  }
}

void ProfileImportHandler::ImporterCleanup() {
  importer_->Cancel();
  importer_ = NULL;
  bridge_ = NULL;
  import_thread_.reset();
  content::UtilityThread::Get()->ReleaseProcessIfNeeded();
}
