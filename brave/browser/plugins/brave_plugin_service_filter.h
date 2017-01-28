// Copyright (c) 2012 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_
#define BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_

#include <string>

#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "content/public/browser/plugin_service_filter.h"
#include "content/public/common/webplugininfo.h"
#include "url/gurl.h"
#include "url/origin.h"

class Profile;

namespace content {
class WebContents;
}

// This class must be created (by calling the |GetInstance| method) on the UI
// thread, but is safe to use on any thread after that.
class BravePluginServiceFilter : public content::PluginServiceFilter {
 public:
  static BravePluginServiceFilter* GetInstance();

  // This method should be called on the UI thread.
  void RegisterResourceContext(Profile* profile, const void* context);

  void UnregisterResourceContext(const void* context);

  // Overrides the plugin lookup mechanism for a given tab and object URL to use
  // a specific plugin.
  void OverridePluginForFrame(int render_process_id,
                              int render_frame_id,
                              const GURL& url,
                              const content::WebPluginInfo& plugin);

  // Authorizes a given plugin for a given process.
  void AuthorizePlugin(int render_process_id,
                       const base::FilePath& plugin_path);

  // Authorizes all plugins for a given WebContents. If |load_blocked| is true,
  // then the renderer is told to load the plugin with given |identifier| (or
  // pllugins if |identifier| is empty).
  // This method can only be called on the UI thread.
  void AuthorizeAllPlugins(content::WebContents* web_contents,
                           bool load_blocked,
                           const std::string& identifier);

  // PluginServiceFilter implementation.
  // If |url| is not available, pass the same URL used to
  // generate |main_frame_origin|. These parameters may be empty.
  bool IsPluginAvailable(int render_process_id,
                         int render_frame_id,
                         const void* context,
                         const GURL& url,
                         const url::Origin& main_frame_origin,
                         content::WebPluginInfo* plugin) override;

  // CanLoadPlugin always grants permission to the browser
  // (render_process_id == 0)
  bool CanLoadPlugin(int render_process_id,
                     const base::FilePath& path) override;

 private:
  friend struct base::DefaultSingletonTraits<BravePluginServiceFilter>;

  BravePluginServiceFilter();
  ~BravePluginServiceFilter() override;
};

#endif  // BRAVE_BROWSER_PLUGINS_BRAVE_PLUGIN_SERVICE_FILTER_H_
