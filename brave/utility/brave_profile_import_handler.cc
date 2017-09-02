// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <utility>

#include "brave/utility/brave_profile_import_handler.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "chrome/utility/importer/importer.h"
#include "brave/utility/importer/importer_creator.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

using chrome::mojom::ThreadSafeProfileImportObserverPtr;

BraveProfileImportHandler::BraveProfileImportHandler() :
    ProfileImportHandler() {}

BraveProfileImportHandler::~BraveProfileImportHandler() {}

// static
void BraveProfileImportHandler::Create(
    mojo::InterfaceRequest<chrome::mojom::ProfileImport> request) {
  mojo::MakeStrongBinding(base::MakeUnique<BraveProfileImportHandler>(),
                          std::move(request));
}

void BraveProfileImportHandler::StartImport(
    const importer::SourceProfile& source_profile,
    uint16_t items,
    std::unique_ptr<base::DictionaryValue> localized_strings,
    chrome::mojom::ProfileImportObserverPtr observer) {
  content::UtilityThread::Get()->EnsureBlinkInitialized();
  importer_ =
      brave_importer::CreateImporterByType(source_profile.importer_type);
  if (!importer_.get()) {
    observer->OnImportFinished(false, "Importer could not be created.");
    return;
  }

  items_to_import_ = items;

  if (base::StartsWith(base::UTF16ToUTF8(source_profile.importer_name),
                       "Chrome", base::CompareCase::SENSITIVE)) {
    auto command_line = base::CommandLine::ForCurrentProcess();
    command_line->AppendSwitch("import-chrome");
  }

  // Create worker thread in which importer runs.
  import_thread_.reset(new base::Thread("import_thread"));
#if defined(OS_WIN)
  import_thread_->init_com_with_mta(false);
#endif
  if (!import_thread_->Start()) {
    NOTREACHED();
    ImporterCleanup();
  }
  bridge_ = new BraveExternalProcessImporterBridge(
      *localized_strings,
      ThreadSafeProfileImportObserverPtr::Create(std::move(observer)));
  import_thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&Importer::StartImport, importer_,
                                source_profile, items,
                                base::RetainedRef(bridge_)));
}
