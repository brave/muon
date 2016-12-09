// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tab_util.h"

#include <stddef.h>
#include <utility>

#include "atom/browser/extensions/tab_helper.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/tabs.h"
#include "chrome/common/url_constants.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/app_window/app_window.h"
#include "extensions/browser/app_window/app_window_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "url/gurl.h"

using content::NavigationEntry;
using content::WebContents;

namespace extensions {

ExtensionTabUtil::OpenTabParams::OpenTabParams()
    : create_browser_if_needed(false) {
}

ExtensionTabUtil::OpenTabParams::~OpenTabParams() {
}

base::DictionaryValue* ExtensionTabUtil::OpenTab(
    ChromeUIThreadExtensionFunction* function,
    const OpenTabParams& params,
    std::string* error) {
  NOTIMPLEMENTED();;
  return NULL;
}

Browser* ExtensionTabUtil::GetBrowserFromWindowID(
    ChromeUIThreadExtensionFunction* function,
    int window_id,
    std::string* error) {
  NOTIMPLEMENTED();
  return NULL;
}

Browser* ExtensionTabUtil::GetBrowserFromWindowID(
    const ChromeExtensionFunctionDetails& details,
    int window_id,
    std::string* error) {
  NOTIMPLEMENTED();
  return NULL;
}

int ExtensionTabUtil::GetWindowId(const Browser* browser) {
  NOTIMPLEMENTED();
  return -1;
}

int ExtensionTabUtil::GetWindowIdOfTabStripModel(
    const TabStripModel* tab_strip_model) {
  NOTIMPLEMENTED();
  return -1;
}

int ExtensionTabUtil::GetTabId(const WebContents* web_contents) {
  return TabHelper::IdForTab(web_contents);
}

std::string ExtensionTabUtil::GetTabStatusText(bool is_loading) {
  return is_loading ? "loading" : "complete";
}

int ExtensionTabUtil::GetWindowIdOfTab(const WebContents* web_contents) {
  return TabHelper::IdForWindowContainingTab(web_contents);
}

// static
std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    WebContents* contents,
    TabStripModel* tab_strip,
    int tab_index,
    const Extension* extension) {
  NOTIMPLEMENTED();
  return NULL;
}

std::unique_ptr<base::ListValue> ExtensionTabUtil::CreateTabList(
    const Browser* browser,
    const Extension* extension) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    content::WebContents* contents,
    TabStripModel* tab_strip,
    int tab_index) {
  NOTIMPLEMENTED();  return NULL;
}

// static
std::unique_ptr<api::tabs::MutedInfo> ExtensionTabUtil::CreateMutedInfo(
    content::WebContents* contents) {
  NOTIMPLEMENTED();
  return NULL;
}

// static
void ExtensionTabUtil::ScrubTabForExtension(const Extension* extension,
                                            content::WebContents* contents,
                                            api::tabs::Tab* tab) {
  NOTIMPLEMENTED();
}

bool ExtensionTabUtil::GetTabStripModel(const WebContents* web_contents,
                                        TabStripModel** tab_strip_model,
                                        int* tab_index) {
  NOTIMPLEMENTED();
  return false;
}

bool ExtensionTabUtil::GetDefaultTab(Browser* browser,
                                     WebContents** contents,
                                     int* tab_id) {
  NOTIMPLEMENTED();
  return false;
}

bool ExtensionTabUtil::GetTabById(int tab_id,
                                  content::BrowserContext* browser_context,
                                  bool include_incognito,
                                  Browser** browser,
                                  TabStripModel** tab_strip,
                                  WebContents** contents,
                                  int* tab_index) {
  if (tab_id == api::tabs::TAB_ID_NONE)
    return false;
  Profile* profile = Profile::FromBrowserContext(browser_context);
  Profile* incognito_profile =
      include_incognito && profile->HasOffTheRecordProfile() ?
          profile->GetOffTheRecordProfile() : NULL;
  WebContents* target_contents = TabHelper::GetTabById(tab_id, profile);
  if (!target_contents)
    return false;

  if (contents)
    *contents = target_contents;

  return true;
}

GURL ExtensionTabUtil::ResolvePossiblyRelativeURL(const std::string& url_string,
                                                  const Extension* extension) {
  GURL url = GURL(url_string);
  if (!url.is_valid())
    url = extension->GetResourceURL(url_string);

  return url;
}

bool ExtensionTabUtil::IsKillURL(const GURL& url) {
  static const char* kill_hosts[] = {
      chrome::kChromeUICrashHost,
      chrome::kChromeUIDelayedHangUIHost,
      chrome::kChromeUIHangUIHost,
      chrome::kChromeUIKillHost,
      chrome::kChromeUIQuitHost,
      chrome::kChromeUIRestartHost,
      content::kChromeUIBrowserCrashHost,
      content::kChromeUIMemoryExhaustHost,
  };

  // Check a fixed-up URL, to normalize the scheme and parse hosts correctly.
  GURL fixed_url =
      url_formatter::FixupURL(url.possibly_invalid_spec(), std::string());
  if (!fixed_url.SchemeIs(content::kChromeUIScheme))
    return false;

  base::StringPiece fixed_host = fixed_url.host_piece();
  for (size_t i = 0; i < arraysize(kill_hosts); ++i) {
    if (fixed_host == kill_hosts[i])
      return true;
  }

  return false;
}

void ExtensionTabUtil::CreateTab(WebContents* web_contents,
                                 const std::string& extension_id,
                                 WindowOpenDisposition disposition,
                                 const gfx::Rect& initial_rect,
                                 bool user_gesture) {
  NOTIMPLEMENTED();
}

// static
void ExtensionTabUtil::ForEachTab(
    const base::Callback<void(WebContents*)>& callback) {
  NOTIMPLEMENTED();
}

// static
WindowController* ExtensionTabUtil::GetWindowControllerOfTab(
    const WebContents* web_contents) {
  NOTIMPLEMENTED();
  return NULL;
}

bool ExtensionTabUtil::OpenOptionsPage(const Extension* extension,
                                       Browser* browser) {
  NOTIMPLEMENTED();
  return false;
}

// static
bool ExtensionTabUtil::BrowserSupportsTabs(Browser* browser) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace extensions
