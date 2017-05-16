// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
#define BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_

#include "chrome/browser/importer/external_process_importer_host.h"

class BraveExternalProcessImporterHost : public ExternalProcessImporterHost {
 public:
  BraveExternalProcessImporterHost();

 private:
  ~BraveExternalProcessImporterHost() override;

  // Launches the utility process that starts the import task, unless bookmark
  // or template model are not yet loaded. If load is not detected, this method
  // will be called when the loading observer sees that model loading is
  // complete.
  void LaunchImportIfReady() override;

  DISALLOW_COPY_AND_ASSIGN(BraveExternalProcessImporterHost);
};

#endif  // BRAVE_BROWSER_IMPORTER_BRAVE_EXTERNAL_PROCESS_IMPORTER_HOST_H_
