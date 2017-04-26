// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "atom/browser/importer/external_process_importer_client.h"

#include "atom/browser/importer/in_process_importer_bridge.h"

namespace atom {

ExternalProcessImporterClient::ExternalProcessImporterClient(
    base::WeakPtr<ExternalProcessImporterHost> importer_host,
    const importer::SourceProfile& source_profile,
    uint16_t items,
    InProcessImporterBridge* bridge)
    : ::ExternalProcessImporterClient(
          importer_host, source_profile, items, bridge),
      total_cookies_count_(0),
      bridge_(bridge),
      cancelled_(false) {}

void ExternalProcessImporterClient::Cancel() {
  if (cancelled_)
    return;

  cancelled_ = true;
  ::ExternalProcessImporterClient::Cancel();
}

void ExternalProcessImporterClient::OnCookiesImportStart(
    uint32_t total_cookies_count) {
  if (cancelled_)
    return;

  total_cookies_count_ = total_cookies_count;
  cookies_.reserve(total_cookies_count);
}
void ExternalProcessImporterClient::OnCookiesImportGroup(
    const std::vector<ImportedCookieEntry>& cookies_group) {
  if (cancelled_)
    return;

  cookies_.insert(cookies_.end(), cookies_group.begin(),
                  cookies_group.end());
  if (cookies_.size() >= total_cookies_count_)
    bridge_->SetCookies(cookies_);
}

ExternalProcessImporterClient::~ExternalProcessImporterClient() {}

}  // namespace atom
