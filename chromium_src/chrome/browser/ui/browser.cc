// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/browser.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <utility>

#include "atom/browser/native_window.h"
#include "build/build_config.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/lifetime/keep_alive_registry.h"
#include "chrome/browser/lifetime/keep_alive_types.h"
#include "chrome/browser/lifetime/scoped_keep_alive.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/notification_service.h"

Browser::CreateParams::CreateParams(Profile* profile, bool user_gesture)
    : type(TYPE_TABBED),
      profile(profile),
      trusted_source(false),
      initial_show_state(ui::SHOW_STATE_DEFAULT),
      is_session_restore(false),
      user_gesture(user_gesture),
      window(NULL) {}

Browser::CreateParams::CreateParams(Type type,
                                    Profile* profile,
				    bool user_gesture)
    : type(type),
      profile(profile),
      trusted_source(false),
      initial_show_state(ui::SHOW_STATE_DEFAULT),
      is_session_restore(false),
      user_gesture(user_gesture),
      window(NULL) {}

Browser::CreateParams::CreateParams(const CreateParams& other) = default;

// static
Browser::CreateParams Browser::CreateParams::CreateForApp(
    const std::string& app_name,
    bool trusted_source,
    const gfx::Rect& window_bounds,
    Profile* profile,
    bool user_gesture) {
  DCHECK(!app_name.empty());

  CreateParams params(TYPE_POPUP, profile, user_gesture);
  params.app_name = app_name;
  params.trusted_source = trusted_source;
  params.initial_bounds = window_bounds;

  return params;
}

Browser::Browser(const CreateParams& params)
    : type_(params.type),
      profile_(params.profile),
      window_(params.window),
      tab_strip_model_(
          new TabStripModel(nullptr, params.profile)),
      app_name_(params.app_name),
      is_trusted_source_(params.trusted_source),
      initial_show_state_(params.initial_show_state),
      is_session_restore_(params.is_session_restore),
      weak_factory_(this) {
  BrowserList::AddBrowser(this);
  content::NotificationService::current()->Notify(
      chrome::NOTIFICATION_BROWSER_WINDOW_READY, content::Source<Browser>(this),
      content::NotificationService::NoDetails());
}

Browser::~Browser() {
  BrowserList::RemoveBrowser(this);
}

bool Browser::is_app() const {
  return !app_name_.empty();
}

bool Browser::ShouldRunUnloadListenerBeforeClosing(
    content::WebContents* web_contents) {
  return false;
}

bool Browser::RunUnloadListenerBeforeClosing(
    content::WebContents* web_contents) {
  return false;
}

void Browser::RegisterKeepAlive() {
  keep_alive_.reset(new ScopedKeepAlive(KeepAliveOrigin::BROWSER,
                                        KeepAliveRestartOption::DISABLED));
}
void Browser::UnregisterKeepAlive() {
  keep_alive_.reset();
}

bool Browser::SupportsWindowFeature(WindowFeature feature) const {
  return SupportsWindowFeatureImpl(feature, true);
}

bool Browser::CanSupportWindowFeature(WindowFeature feature) const {
  return SupportsWindowFeatureImpl(feature, false);
}

bool Browser::TryToCloseWindow(bool skip_beforeunload,
    const base::Callback<void(bool)>& on_close_confirmed) {
  on_close_confirmed.Run(true);
  return false;
}

void Browser::ResetTryToCloseWindow() {
}

bool Browser::SupportsWindowFeatureImpl(WindowFeature feature,
                                        bool check_fullscreen) const {
  unsigned int features = FEATURE_NONE;
    if (is_type_tabbed())
      features |= FEATURE_TABSTRIP;
  return !!(features & feature);
}
