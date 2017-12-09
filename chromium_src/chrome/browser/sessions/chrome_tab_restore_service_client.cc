// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sessions/chrome_tab_restore_service_client.h"

#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

ChromeTabRestoreServiceClient::ChromeTabRestoreServiceClient(Profile* profile) :
  profile_(profile) {
}

ChromeTabRestoreServiceClient::~ChromeTabRestoreServiceClient() {}

sessions::LiveTabContext*
ChromeTabRestoreServiceClient::FindLiveTabContextWithID(int) {
  NOTIMPLEMENTED();
  return nullptr;
}

base::FilePath ChromeTabRestoreServiceClient::GetPathToSaveTo() {
  return profile_->GetPath();
}

bool ChromeTabRestoreServiceClient::HasLastSession() {
  return false;
}

void ChromeTabRestoreServiceClient::GetLastSession(
    const sessions::GetLastSessionCallback& callback,
    base::CancelableTaskTracker* tracker) {}

sessions::LiveTabContext*
ChromeTabRestoreServiceClient::FindLiveTabContextForTab(
    const sessions::LiveTab* tab) {
  NOTIMPLEMENTED();
  return nullptr;
}

sessions::LiveTabContext* ChromeTabRestoreServiceClient::CreateLiveTabContext(
    const std::string& app_name) {
  NOTIMPLEMENTED();
  return nullptr;
}

GURL ChromeTabRestoreServiceClient::GetNewTabURL() {
  return GURL("about:newtab");
}

bool ChromeTabRestoreServiceClient::ShouldTrackURLForRestore(const GURL& url) {
  return false;
}

std::string ChromeTabRestoreServiceClient::GetExtensionAppIDForTab(
    sessions::LiveTab* tab) {
  return std::string();
}

void ChromeTabRestoreServiceClient::OnTabRestored(const GURL& url) {
  NOTREACHED();
}
