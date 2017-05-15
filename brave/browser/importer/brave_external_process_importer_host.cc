// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/importer/brave_external_process_importer_host.h"

#include "atom/browser/importer/external_process_importer_client.h"
#include "atom/browser/importer/in_process_importer_bridge.h"

BraveExternalProcessImporterHost::BraveExternalProcessImporterHost()
  : ExternalProcessImporterHost() {}

void BraveExternalProcessImporterHost::LaunchImportIfReady() {
  if (waiting_for_bookmarkbar_model_ || template_service_subscription_.get() ||
      !is_source_readable_ || cancelled_)
    return;

  // This is the in-process half of the bridge, which catches data from the IPC
  // pipe and feeds it to the ProfileWriter. The external process half of the
  // bridge lives in the external process (see ProfileImportThread).
  // The ExternalProcessImporterClient created in the next line owns the bridge,
  // and will delete it.
  atom::InProcessImporterBridge* bridge =
      new atom::InProcessImporterBridge(writer_.get(),
                                  weak_ptr_factory_.GetWeakPtr());
  client_ = new atom::ExternalProcessImporterClient(
      weak_ptr_factory_.GetWeakPtr(), source_profile_, items_, bridge);
  client_->Start();
}

BraveExternalProcessImporterHost::~BraveExternalProcessImporterHost() {}
