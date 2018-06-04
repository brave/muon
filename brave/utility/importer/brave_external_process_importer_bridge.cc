// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <utility>

#include "brave/utility/importer/brave_external_process_importer_bridge.h"

#include "base/logging.h"
#include "build/build_config.h"

using chrome::mojom::ProfileImportObserver;

namespace {

const int kNumCookiesToSend = 100;

}  // namespace

void BraveExternalProcessImporterBridge::SetCookies(
    const std::vector<ImportedCookieEntry>& cookies) {
  (*observer_)->OnCookiesImportStart(cookies.size());

  // |cookies_left| is required for the checks below as Windows has a
  // Debug bounds-check which prevents pushing an iterator beyond its end()
  // (i.e., |it + 2 < s.end()| crashes in debug mode if |i + 1 == s.end()|).
  int cookies_left = cookies.end() - cookies.begin();
  for (std::vector<ImportedCookieEntry>::const_iterator it =
           cookies.begin(); it < cookies.end();) {
    std::vector<ImportedCookieEntry> cookies_group;
    std::vector<ImportedCookieEntry>::const_iterator end_group =
        it + std::min(cookies_left, kNumCookiesToSend);
    cookies_group.assign(it, end_group);

    (*observer_)->OnCookiesImportGroup(cookies_group);
    cookies_left -= end_group - it;
    it = end_group;
  }
  DCHECK_EQ(0, cookies_left);
}

BraveExternalProcessImporterBridge::BraveExternalProcessImporterBridge(
    const base::flat_map<uint32_t, std::string>& localized_strings,
    scoped_refptr<chrome::mojom::ThreadSafeProfileImportObserverPtr> observer)
    : ExternalProcessImporterBridge(std::move(localized_strings), observer) {}

BraveExternalProcessImporterBridge::~BraveExternalProcessImporterBridge() {}
