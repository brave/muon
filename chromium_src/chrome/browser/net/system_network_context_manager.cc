// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/system_network_context_manager.h"

#include "chrome/browser/net/default_network_context_params.h"

SystemNetworkContextManager::SystemNetworkContextManager() {}
SystemNetworkContextManager::~SystemNetworkContextManager() {}

content::mojom::NetworkContextParamsPtr CreateNetworkContextParams() {
  // TODO(mmenke): Set up parameters here (in memory cookie store, etc).
  content::mojom::NetworkContextParamsPtr network_context_params =
      CreateDefaultNetworkContextParams();

  network_context_params->context_name = std::string("system");

  network_context_params->http_cache_enabled = false;

  // These are needed for PAC scripts that use file or data URLs (Or FTP URLs?).
  network_context_params->enable_data_url_support = true;
  network_context_params->enable_file_url_support = true;
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif

  return network_context_params;
}

void SystemNetworkContextManager::SetUp(
    content::mojom::NetworkContextRequest* network_context_request,
    content::mojom::NetworkContextParamsPtr* network_context_params,
    bool* is_quic_allowed) {
  *network_context_request = mojo::MakeRequest(&io_thread_network_context_);
  *network_context_params = CreateNetworkContextParams();
  *is_quic_allowed = is_quic_allowed_;
}
