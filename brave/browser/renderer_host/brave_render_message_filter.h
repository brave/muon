// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_RENDERER_HOST_BRAVE_RENDER_MESSAGE_FILTER_H_
#define BRAVE_BROWSER_RENDERER_HOST_BRAVE_RENDER_MESSAGE_FILTER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/sequenced_task_runner_helpers.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/buildflags/buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"

class GURL;
class Profile;

namespace content_settings {
class CookieSettings;
}

// This class filters out incoming Chrome-specific IPC messages for the renderer
// process on the IPC thread.
class BraveRenderMessageFilter : public content::BrowserMessageFilter {
 public:
  BraveRenderMessageFilter(int render_process_id, Profile* profile);

  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  friend class content::BrowserThread;
  friend class base::DeleteHelper<BraveRenderMessageFilter>;

  ~BraveRenderMessageFilter() override;

  void OnAllowDatabase(int render_frame_id,
                       const GURL& origin_url,
                       const GURL& top_origin_url,
                       const base::string16& name,
                       const base::string16& display_name,
                       bool* allowed);
  void OnAllowDOMStorage(int render_frame_id,
                         const GURL& origin_url,
                         const GURL& top_origin_url,
                         bool local,
                         bool* allowed);
  void OnAllowIndexedDB(int render_frame_id,
                        const GURL& origin_url,
                        const GURL& top_origin_url,
                        const base::string16& name,
                        bool* allowed);

  const int render_process_id_;

  // The Profile associated with our renderer process.  This should only be
  // accessed on the UI thread!
  Profile* profile_;

  HostContentSettingsMap* host_content_settings_map_;

  DISALLOW_COPY_AND_ASSIGN(BraveRenderMessageFilter);
};

#endif  // BRAVE_BROWSER_RENDERER_HOST_BRAVE_RENDER_MESSAGE_FILTER_H_
