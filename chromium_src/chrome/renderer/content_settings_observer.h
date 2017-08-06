// Copyright 2015 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
#define CHROME_RENDERER_CONTENT_SETTINGS_OBSERVER_H_

#include <map>
#include <set>

#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "third_party/WebKit/public/platform/WebContentSettingsClient.h"

class GURL;

namespace blink {
class WebFrame;
class WebSecurityOrigin;
class WebURL;
}

namespace extensions {
class Dispatcher;
}

namespace atom {
class ContentSettingsManager;
}

// Handles blocking content per content settings for each RenderFrame.
class ContentSettingsObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<ContentSettingsObserver>,
      public blink::WebContentSettingsClient {
 public:
  ContentSettingsObserver(content::RenderFrame* render_frame,
                          extensions::Dispatcher* extension_dispatcher,
                          bool should_whitelist);
  ~ContentSettingsObserver() override;

  void SetContentSettingRules(
      const RendererContentSettingRules* content_setting_rules);

  void SetContentSettingsManager(
      atom::ContentSettingsManager* content_settings_manager);

  bool IsPluginTemporarilyAllowed(const std::string& identifier);

  // Sends an IPC notification that the specified content type was blocked.
  void DidBlockContentType(ContentSettingsType settings_type);

  // Sends an IPC notification that the specified content type was blocked
  // with additional metadata.
  void DidBlockContentType(ContentSettingsType settings_type,
                           const base::string16& details);

  // Sends an IPC notification that the specified content type was blocked.
  void DidBlockContentType(const std::string& settings_type);

  // Sends an IPC notification that the specified content type was blocked
  // with additional metadata.
  void DidBlockContentType(const std::string& settings_type,
                           const std::string& details);

  // blink::WebContentSettingsClient implementation.
  bool AllowDatabase(const blink::WebString& name,
                     const blink::WebString& display_name,
                     unsigned estimated_size) override;  // NOLINT
  void RequestFileSystemAccessAsync(
          const blink::WebContentSettingCallbacks& callbacks) override;
  bool AllowImage(bool enabled_per_settings,
                  const blink::WebURL& image_url) override;
  bool AllowIndexedDB(const blink::WebString& name,
                      const blink::WebSecurityOrigin& origin) override;
  bool AllowScript(bool enabled_per_settings) override;
  bool AllowScriptFromSource(bool enabled_per_settings,
                             const blink::WebURL& script_url) override;
  bool AllowStorage(bool local) override;
  bool AllowReadFromClipboard(bool default_value) override;
  bool AllowWriteToClipboard(bool default_value) override;
  bool AllowMutationEvents(bool default_value) override;
  bool AllowRunningInsecureContent(bool allowed_per_settings,
                                   const blink::WebSecurityOrigin& context,
                                   const blink::WebURL& url) override;
  bool AllowAutoplay(bool defaultValue) override;
  void DidNotAllowPlugins() override;
  void DidNotAllowScript() override;

 private:
  void DidRunInsecureContent(GURL resouce_url);
  void DidBlockRunInsecureContent(GURL resouce_url);

  // RenderFrameObserver implementation.
  bool OnMessageReceived(const IPC::Message& message) override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_page_navigation) override;
  void OnDestruct() override;
  // Resets the |content_blocked_| array.
  void ClearBlockedContentSettings();

  void OnLoadBlockedPlugins(const std::string& identifier);

  // Helpers.
  // True if |render_frame()| contains content that is white-listed for content
  // settings.
  bool IsWhitelistedForContentSettings() const;
  static bool IsWhitelistedForContentSettings(
      const blink::WebSecurityOrigin& origin,
      const GURL& document_url);

  // Owned by ChromeContentRendererClient and outlive us.
  extensions::Dispatcher* extension_dispatcher_;
  atom::ContentSettingsManager* content_settings_manager_;  // not owned

  // Insecure content may be permitted for the duration of this render view.
  bool allow_running_insecure_content_;

  // A pointer to content setting rules stored by the renderer. Normally, the
  // |RendererContentSettingRules| object is owned by
  // |ChromeRenderThreadObserver|. In the tests it is owned by the caller of
  // |SetContentSettingRules|.
  const RendererContentSettingRules* content_setting_rules_;

  // Stores if images, scripts, and plugins have actually been blocked.
  std::map<ContentSettingsType, bool> content_blocked_;

  // Caches the result of AllowStorage.
  typedef std::pair<GURL, bool> StoragePermissionsKey;
  std::map<StoragePermissionsKey, bool> cached_storage_permissions_;

  // Caches the result of AllowScript.
  std::map<blink::WebFrame*, bool> cached_script_permissions_;

  std::set<std::string> temporarily_allowed_plugins_;
  bool is_interstitial_page_;

  int current_request_id_;
  typedef std::map<int, blink::WebContentSettingCallbacks> PermissionRequestMap;
  PermissionRequestMap permission_requests_;

  // If true, IsWhitelistedForContentSettings will always return true.
  const bool should_whitelist_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsObserver);
};

#endif  // CHROME_RENDERER_CONTENT_SETTINGS_OBSERVER_H_
