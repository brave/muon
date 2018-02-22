// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_tab_util.h"

#include <stddef.h>

#include "atom/browser/extensions/tab_helper.h"
#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "chrome/browser/resource_coordinator/tab_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tab_contents/tab_contents_iterator.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
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

namespace {

namespace keys = tabs_constants;

// |error_message| can optionally be passed in and will be set with an
// appropriate message if the window cannot be found by id.
Browser* GetBrowserInProfileWithId(Profile* profile,
                                   const int window_id,
                                   bool include_incognito,
                                   std::string* error_message) {
  Profile* incognito_profile =
      include_incognito && profile->HasOffTheRecordProfile()
          ? profile->GetOffTheRecordProfile()
          : NULL;
  for (auto* browser : *BrowserList::GetInstance()) {
    if ((browser->profile() == profile ||
         browser->profile() == incognito_profile) &&
        ExtensionTabUtil::GetWindowId(browser) == window_id &&
        browser->window()) {
      return browser;
    }
  }

  if (error_message)
    *error_message = ErrorUtils::FormatErrorMessage(
        keys::kWindowNotFoundError, base::IntToString(window_id));

  return NULL;
}

// Use this function for reporting a tab id to an extension. It will
// take care of setting the id to TAB_ID_NONE if necessary (for
// example with devtools).
int GetTabIdForExtensions(const WebContents* web_contents) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  if (browser && !ExtensionTabUtil::BrowserSupportsTabs(browser))
    return -1;
  return SessionTabHelper::IdForTab(web_contents);
}

}  // namespace

ExtensionTabUtil::OpenTabParams::OpenTabParams()
    : create_browser_if_needed(false) {
}

ExtensionTabUtil::OpenTabParams::~OpenTabParams() {
}

int ExtensionTabUtil::GetWindowId(const Browser* browser) {
  return const_cast<Browser*>(browser)->session_id().id();
}

int ExtensionTabUtil::GetWindowIdOfTabStripModel(
    const TabStripModel* tab_strip_model) {
  for (auto* browser : *BrowserList::GetInstance()) {
    if (browser->tab_strip_model() == tab_strip_model)
      return GetWindowId(browser);
  }
  return -1;
}

int ExtensionTabUtil::GetTabId(const WebContents* web_contents) {
  return SessionTabHelper::IdForTab(web_contents);
}

std::string ExtensionTabUtil::GetTabStatusText(bool is_loading) {
  return is_loading ? keys::kStatusValueLoading : keys::kStatusValueComplete;
}

int ExtensionTabUtil::GetWindowIdOfTab(const WebContents* web_contents) {
  return SessionTabHelper::IdForWindowContainingTab(web_contents);
}

// static
std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    WebContents* contents,
    TabStripModel* tab_strip,
    int tab_index,
    const Extension* extension) {
  std::unique_ptr<api::tabs::Tab> result =
      CreateTabObject(contents, tab_strip, tab_index);
  ScrubTabForExtension(extension, contents, result.get());
  return result;
}

std::unique_ptr<base::ListValue> ExtensionTabUtil::CreateTabList(
    const Browser* browser,
    const Extension* extension) {
  std::unique_ptr<base::ListValue> tab_list(new base::ListValue());
  TabStripModel* tab_strip = browser->tab_strip_model();
  for (int i = 0; i < tab_strip->count(); ++i) {
    tab_list->Append(
        CreateTabObject(tab_strip->GetWebContentsAt(i), tab_strip, i, extension)
            ->ToValue());
  }

  return tab_list;
}

// static
std::unique_ptr<api::tabs::Tab> ExtensionTabUtil::CreateTabObject(
    content::WebContents* contents,
    TabStripModel* tab_strip,
    int tab_index) {
  if (!tab_strip)
    ExtensionTabUtil::GetTabStripModel(contents, &tab_strip, &tab_index);

  auto tab_helper = TabHelper::FromWebContents(contents);

  bool is_loading = contents->IsLoading();
  std::unique_ptr<api::tabs::Tab> tab_object(new api::tabs::Tab);
  tab_object->id.reset(new int(GetTabIdForExtensions(contents)));
  tab_object->index = tab_helper->get_index();
  tab_object->window_id = GetWindowIdOfTab(contents);
  tab_object->status.reset(new std::string(GetTabStatusText(is_loading)));
  tab_object->active = tab_helper->is_active();
  tab_object->selected = tab_object->active;
  tab_object->highlighted = tab_strip && tab_strip->IsTabSelected(tab_index);
  tab_object->pinned = tab_helper->is_pinned();
  tab_object->audible.reset(new bool(contents->WasRecentlyAudible()));
  tab_object->discarded = tab_helper->IsDiscarded();
  tab_object->auto_discardable =
      g_browser_process->GetTabManager()->IsTabAutoDiscardable(contents);
  tab_object->muted_info = CreateMutedInfo(contents);
  tab_object->incognito = contents->GetBrowserContext()->IsOffTheRecord();
  tab_object->width.reset(
      new int(contents->GetContainerBounds().size().width()));
  tab_object->height.reset(
      new int(contents->GetContainerBounds().size().height()));

  tab_object->url.reset(new std::string(contents->GetURL().spec()));
  tab_object->title.reset(
      new std::string(base::UTF16ToUTF8(contents->GetTitle())));
  NavigationEntry* entry = contents->GetController().GetVisibleEntry();
  if (entry && entry->GetFavicon().valid)
    tab_object->fav_icon_url.reset(
        new std::string(entry->GetFavicon().url.spec()));

  tab_object->opener_tab_id.reset(new int(tab_helper->opener_tab_id()));

  return tab_object;
}

// static
std::unique_ptr<api::tabs::MutedInfo> ExtensionTabUtil::CreateMutedInfo(
    content::WebContents* contents) {
  DCHECK(contents);
  std::unique_ptr<api::tabs::MutedInfo> info(new api::tabs::MutedInfo);
  info->muted = contents->IsAudioMuted();
  info->reason = api::tabs::MUTED_INFO_REASON_USER;
  return info;
}

// static
void ExtensionTabUtil::ScrubTabForExtension(const Extension* extension,
                                            content::WebContents* contents,
                                            api::tabs::Tab* tab) {
  bool has_permission = false;
  if (extension) {
    bool api_permission = false;
    std::string url;
    if (contents) {
      api_permission = extension->permissions_data()->HasAPIPermissionForTab(
          GetTabId(contents), APIPermission::kTab);
      url = contents->GetURL().spec();
    } else {
      api_permission =
          extension->permissions_data()->HasAPIPermission(APIPermission::kTab);
      url = *tab->url;
    }
    bool host_permission = extension->permissions_data()
                               ->active_permissions()
                               .HasExplicitAccessToOrigin(GURL(url));
    has_permission = api_permission || host_permission;
  }
  if (!has_permission) {
    tab->url.reset();
    tab->title.reset();
    tab->fav_icon_url.reset();
  }
}

bool ExtensionTabUtil::GetTabStripModel(const WebContents* web_contents,
                                        TabStripModel** tab_strip_model,
                                        int* tab_index) {
  DCHECK(web_contents);
  DCHECK(tab_strip_model);
  DCHECK(tab_index);

  for (auto* browser : *BrowserList::GetInstance()) {
    TabStripModel* tab_strip = browser->tab_strip_model();
    int index = tab_strip->GetIndexOfWebContents(web_contents);
    if (index != -1) {
      *tab_strip_model = tab_strip;
      *tab_index = index;
      return true;
    }
  }

  return false;
}

bool ExtensionTabUtil::GetDefaultTab(Browser* browser,
                                     WebContents** contents,
                                     int* tab_id) {
  DCHECK(browser);
  DCHECK(contents);

  *contents = browser->tab_strip_model()->GetActiveWebContents();
  if (*contents) {
    if (tab_id)
      *tab_id = GetTabId(*contents);
    return true;
  }

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
  for (auto* target_browser : *BrowserList::GetInstance()) {
    if (target_browser->profile() == profile ||
        target_browser->profile() == incognito_profile) {
      TabStripModel* target_tab_strip = target_browser->tab_strip_model();
      for (int i = 0; i < target_tab_strip->count(); ++i) {
        WebContents* target_contents = target_tab_strip->GetWebContentsAt(i);
        if (SessionTabHelper::IdForTab(target_contents) == tab_id) {
          if (browser)
            *browser = target_browser;
          if (tab_strip)
            *tab_strip = target_tab_strip;
          if (contents)
            *contents = target_contents;
          if (tab_index)
            *tab_index = i;
          return true;
        }
      }
    }
  }
  return false;
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

// static
void ExtensionTabUtil::ForEachTab(
    const base::Callback<void(WebContents*)>& callback) {
  for (auto* web_contents : AllTabContentses())
    callback.Run(web_contents);

}

// static
bool ExtensionTabUtil::BrowserSupportsTabs(Browser* browser) {
  return browser && browser->tab_strip_model() && !browser->is_devtools();
}

}  // namespace extensions
