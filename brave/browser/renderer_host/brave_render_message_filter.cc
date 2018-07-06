// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/renderer_host/brave_render_message_filter.h"

#include <stdint.h>

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/macros.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/render_messages.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"

using content::BrowserThread;

namespace {

const uint32_t kRenderFilteredMessageClasses[] = {
    ChromeMsgStart,
};

}  // namespace

BraveRenderMessageFilter::BraveRenderMessageFilter(int render_process_id,
                                                     Profile* profile)
    : BrowserMessageFilter(kRenderFilteredMessageClasses,
                           arraysize(kRenderFilteredMessageClasses)),
      render_process_id_(render_process_id),
      profile_(profile) {
  host_content_settings_map_ =
    HostContentSettingsMapFactory::GetForProfile(profile);
}

BraveRenderMessageFilter::~BraveRenderMessageFilter() {
}

bool BraveRenderMessageFilter::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BraveRenderMessageFilter, message)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowDatabase, OnAllowDatabase)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowDOMStorage, OnAllowDOMStorage)
    IPC_MESSAGE_HANDLER(ChromeViewHostMsg_AllowIndexedDB, OnAllowIndexedDB)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void BraveRenderMessageFilter::OnAllowDatabase(
    int render_frame_id,
    const GURL& origin_url,
    const GURL& top_origin_url,
    const base::string16& name,
    const base::string16& display_name,
    bool* allowed) {
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
    origin_url, top_origin_url, CONTENT_SETTINGS_TYPE_COOKIES, "");
  *allowed = setting == ContentSetting::CONTENT_SETTING_ALLOW;
}

void BraveRenderMessageFilter::OnAllowDOMStorage(int render_frame_id,
                                                  const GURL& origin_url,
                                                  const GURL& top_origin_url,
                                                  bool local,
                                                  bool* allowed) {
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
    origin_url, top_origin_url, CONTENT_SETTINGS_TYPE_COOKIES, "");
  *allowed = setting == ContentSetting::CONTENT_SETTING_ALLOW;
}

void BraveRenderMessageFilter::OnAllowIndexedDB(int render_frame_id,
                                                 const GURL& origin_url,
                                                 const GURL& top_origin_url,
                                                 const base::string16& name,
                                                 bool* allowed) {
  ContentSetting setting = host_content_settings_map_->GetContentSetting(
    origin_url, top_origin_url, CONTENT_SETTINGS_TYPE_COOKIES, "");
  *allowed = setting == ContentSetting::CONTENT_SETTING_ALLOW;
}

