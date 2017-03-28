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
#include "chrome/browser/devtools/devtools_window.h"
#include "chrome/browser/lifetime/keep_alive_registry.h"
#include "chrome/browser/lifetime/keep_alive_types.h"
#include "chrome/browser/lifetime/scoped_keep_alive.h"
#include "chrome/browser/ui/browser_list.h"
// #include "chrome/browser/ui/browser_tab_strip_model_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"

Browser::CreateParams::CreateParams(Profile* profile)
    : type(TYPE_TABBED),
      profile(profile),
      trusted_source(false),
      initial_show_state(ui::SHOW_STATE_DEFAULT),
      is_session_restore(false),
      window(NULL) {}

Browser::CreateParams::CreateParams(Type type, Profile* profile)
    : type(type),
      profile(profile),
      trusted_source(false),
      initial_show_state(ui::SHOW_STATE_DEFAULT),
      is_session_restore(false),
      window(NULL) {}

Browser::CreateParams::CreateParams(const CreateParams& other) = default;

// TODO(bridiver) - atom_api_web_contents should be tab strip model delegate
Browser::Browser(const CreateParams& params)
    : type_(params.type),
      profile_(params.profile),
      window_(params.window),
      // tab_strip_model_delegate_(new chrome::BrowserTabStripModelDelegate(this)),
      tab_strip_model_(
          new TabStripModel(nullptr, params.profile)),
      app_name_(params.app_name),
      is_trusted_source_(params.trusted_source),
      initial_show_state_(params.initial_show_state),
      is_session_restore_(params.is_session_restore),
      weak_factory_(this) {
  BrowserList::AddBrowser(this);
  // content::NotificationService::current()->Notify(
  //     chrome::NOTIFICATION_BROWSER_WINDOW_READY, content::Source<Browser>(this),
  //     content::NotificationService::NoDetails());
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

bool Browser::CallBeforeUnloadHandlers(
    const base::Callback<void(bool)>& on_close_confirmed) {
  on_close_confirmed.Run(true);
}

void Browser::ResetBeforeUnloadHandlers() {
}

bool Browser::SupportsWindowFeatureImpl(WindowFeature feature,
                                        bool check_fullscreen) const {
//   bool hide_ui_for_fullscreen = check_fullscreen && ShouldHideUIForFullscreen();

  unsigned int features = FEATURE_NONE;

//   if (is_type_tabbed())
//     features |= FEATURE_BOOKMARKBAR;

//   if (!hide_ui_for_fullscreen) {
//     if (!is_type_tabbed())
//       features |= FEATURE_TITLEBAR;

    if (is_type_tabbed())
      features |= FEATURE_TABSTRIP;

//     if (is_type_tabbed())
//       features |= FEATURE_TOOLBAR;

    // if (SupportsLocationBar())
    //   features |= FEATURE_LOCATIONBAR;

//     if (ShouldUseWebAppFrame())
//       features |= FEATURE_WEBAPPFRAME;
//   }
  return !!(features & feature);
}
