// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chrome_service.h"

#include "components/spellcheck/spellcheck_build_features.h"
#include "components/startup_metric_utils/browser/startup_metric_host_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "chrome/browser/spellchecker/spell_check_host_chrome_impl.h"
#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
#include "chrome/browser/spellchecker/spell_check_panel_host_impl.h"
#endif
#endif

// static
std::unique_ptr<service_manager::Service> ChromeService::Create() {
  return base::MakeUnique<ChromeService>();
}

ChromeService::ChromeService() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
#if BUILDFLAG(ENABLE_SPELLCHECK)
  registry_with_source_info_.AddInterface(
      base::Bind(&SpellCheckHostChromeImpl::Create),
      content::BrowserThread::GetTaskRunnerForThread(
          content::BrowserThread::UI));
#if BUILDFLAG(HAS_SPELLCHECK_PANEL)
  registry_.AddInterface(base::Bind(&SpellCheckPanelHostImpl::Create),
                         content::BrowserThread::GetTaskRunnerForThread(
                             content::BrowserThread::UI));
#endif
#endif
}

ChromeService::~ChromeService() {}

void ChromeService::OnBindInterface(
    const service_manager::BindSourceInfo& remote_info,
    const std::string& name,
    mojo::ScopedMessagePipeHandle handle) {
  content::OverrideOnBindInterface(remote_info, name, &handle);
  if (!handle.is_valid())
    return;

  if (!registry_.TryBindInterface(name, &handle))
    registry_with_source_info_.TryBindInterface(name, &handle, remote_info);
}
