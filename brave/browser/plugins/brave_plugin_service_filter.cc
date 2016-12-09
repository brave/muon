// Copyright (c) 2012 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/plugins/brave_plugin_service_filter.h"

#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/webplugininfo.h"

#include "content/public/browser/render_frame_host.h"

// static
BravePluginServiceFilter* BravePluginServiceFilter::GetInstance() {
  // also initialize the chrome plugin service filter
  ChromePluginServiceFilter::GetInstance();
  return base::Singleton<BravePluginServiceFilter>::get();
}

BravePluginServiceFilter::BravePluginServiceFilter() {
}

BravePluginServiceFilter::~BravePluginServiceFilter() {
}

void BravePluginServiceFilter::RegisterResourceContext(
    Profile* profile, const void* context) {
  ChromePluginServiceFilter::GetInstance()->
      RegisterResourceContext(profile, context);
}

void BravePluginServiceFilter::UnregisterResourceContext(const void* context) {
  ChromePluginServiceFilter::GetInstance()->
      UnregisterResourceContext(context);
}

void BravePluginServiceFilter::OverridePluginForFrame(int render_process_id,
                            int render_frame_id,
                            const GURL& url,
                            const content::WebPluginInfo& plugin) {
  ChromePluginServiceFilter::GetInstance()->
      OverridePluginForFrame(render_process_id, render_frame_id, url, plugin);
}

void BravePluginServiceFilter::AuthorizePlugin(int render_process_id,
                     const base::FilePath& plugin_path) {
  ChromePluginServiceFilter::GetInstance()->
      AuthorizePlugin(render_process_id, plugin_path);
}

void BravePluginServiceFilter::AuthorizeAllPlugins(
    content::WebContents* web_contents,
    bool load_blocked,
    const std::string& identifier) {
  ChromePluginServiceFilter::GetInstance()->
      AuthorizeAllPlugins(web_contents, load_blocked, identifier);
}

bool BravePluginServiceFilter::IsPluginAvailable(
    int render_process_id,
    int render_frame_id,
    const void* context,
    const GURL& url,
    const GURL& policy_url,
    content::WebPluginInfo* plugin) {
  if (url == GURL() && policy_url == GURL()) {
    return CanLoadPlugin(render_process_id, plugin->path);
  } else {
    return ChromePluginServiceFilter::GetInstance()->IsPluginAvailable(
        render_process_id, render_frame_id, context, url, policy_url, plugin);
  }
}

bool BravePluginServiceFilter::CanLoadPlugin(int render_process_id,
                   const base::FilePath& path) {
  return ChromePluginServiceFilter::GetInstance()->
      CanLoadPlugin(render_process_id, path);
}

