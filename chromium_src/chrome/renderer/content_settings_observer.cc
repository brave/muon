// Copyright 2015 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file provides Brave specific functionality that overrides the
// functionality in Chrome. It is not a copy.
#include "chrome/renderer/content_settings_observer.h"

#include <string>

#include "atom/common/api/api_messages.h"
#include "atom/renderer/content_settings_manager.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/render_messages.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_content_setting_callbacks.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_local_frame_client.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_view.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/permissions/api_permission.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/renderer/dispatcher.h"
#include "extensions/renderer/renderer_extension_registry.h"
#endif

using atom::ContentSettingsManager;
using blink::WebContentSettingCallbacks;
using blink::WebDocument;
using blink::WebFrame;
using blink::WebSecurityOrigin;
using blink::WebString;
using blink::WebURL;
using blink::WebView;
using content::DocumentState;
using content::NavigationState;

ContentSettingsObserver::ContentSettingsObserver(
    content::RenderFrame* render_frame,
    extensions::Dispatcher* extension_dispatcher,
    bool should_whitelist,
    service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<ContentSettingsObserver>(
          render_frame),
#if BUILDFLAG(ENABLE_EXTENSIONS)
      extension_dispatcher_(extension_dispatcher),
#endif
      content_settings_manager_(NULL),
      allow_running_insecure_content_(false),
      is_interstitial_page_(false),
      current_request_id_(0),
      should_whitelist_(should_whitelist) {
  ClearBlockedContentSettings();
  render_frame->GetWebFrame()->SetContentSettingsClient(this);

  content::RenderFrame* main_frame =
      render_frame->GetRenderView()->GetMainRenderFrame();
  // TODO(nasko): The main frame is not guaranteed to be in the same process
  // with this frame with --site-per-process. This code needs to be updated
  // to handle this case. See https://crbug.com/496670.
  if (main_frame && main_frame != render_frame) {
    // Copy all the settings from the main render frame to avoid race conditions
    // when initializing this data. See https://crbug.com/333308.
    ContentSettingsObserver* parent = ContentSettingsObserver::Get(main_frame);
    allow_running_insecure_content_ = parent->allow_running_insecure_content_;
    temporarily_allowed_plugins_ = parent->temporarily_allowed_plugins_;
    is_interstitial_page_ = parent->is_interstitial_page_;
  }
}

ContentSettingsObserver::~ContentSettingsObserver() {
}

void ContentSettingsObserver::SetContentSettingRules(
    const RendererContentSettingRules* content_setting_rules) {
  content_setting_rules_ = content_setting_rules;
}

void ContentSettingsObserver::SetContentSettingsManager(
    atom::ContentSettingsManager* content_settings_manager) {
  content_settings_manager_ = content_settings_manager;
}

bool ContentSettingsObserver::IsPluginTemporarilyAllowed(
    const std::string& identifier) {
  // If the empty string is in here, it means all plugins are allowed.
  // TODO(bauerb): Remove this once we only pass in explicit identifiers.
  return (temporarily_allowed_plugins_.find(identifier) !=
          temporarily_allowed_plugins_.end()) ||
         (temporarily_allowed_plugins_.find(std::string()) !=
          temporarily_allowed_plugins_.end());
}

void ContentSettingsObserver::DidBlockContentType(
    ContentSettingsType settings_type, const base::string16& details) {
  std::string settings_type_string = "unknown";
  switch (settings_type) {
    case CONTENT_SETTINGS_TYPE_COOKIES:
      settings_type_string = "cookies";
      break;
    case CONTENT_SETTINGS_TYPE_IMAGES:
      settings_type_string = "images";
      break;
    case CONTENT_SETTINGS_TYPE_JAVASCRIPT:
      settings_type_string = "javascript";
      break;
    case CONTENT_SETTINGS_TYPE_PLUGINS:
      settings_type_string = "plugins";
      break;
    case CONTENT_SETTINGS_TYPE_POPUPS:
      settings_type_string = "popups";
      break;
    case CONTENT_SETTINGS_TYPE_GEOLOCATION:
      settings_type_string = "geo";
      break;
    case CONTENT_SETTINGS_TYPE_NOTIFICATIONS:
      settings_type_string = "notifications";
      break;
    case CONTENT_SETTINGS_TYPE_AUTO_SELECT_CERTIFICATE:
      settings_type_string = "auto_select_certificate";
      break;
    case CONTENT_SETTINGS_TYPE_MIXEDSCRIPT:
      settings_type_string = "runInsecureContent";
      break;
    case CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC:
      settings_type_string = "mediastream_mic";
      break;
    case CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA:
      settings_type_string = "mediastream_camera";
      break;
    case CONTENT_SETTINGS_TYPE_PROTOCOL_HANDLERS:
      settings_type_string = "protocol_handlers";
      break;
    case CONTENT_SETTINGS_TYPE_PPAPI_BROKER:
      settings_type_string = "ppapi_broker";
      break;
    case CONTENT_SETTINGS_TYPE_AUTOMATIC_DOWNLOADS:
      settings_type_string = "automatic_downloads";
      break;
    case CONTENT_SETTINGS_TYPE_MIDI_SYSEX:
      settings_type_string = "midi_sysex";
      break;
    case CONTENT_SETTINGS_TYPE_SSL_CERT_DECISIONS:
      settings_type_string = "ssl_cert_decisions";
      break;
    case CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER:
      settings_type_string = "protected_media_identifiers";
      break;
    case CONTENT_SETTINGS_TYPE_SITE_ENGAGEMENT:
      settings_type_string = "site_engagement";
      break;
    case CONTENT_SETTINGS_TYPE_DURABLE_STORAGE:
      settings_type_string = "durable_storage";
      break;
    case CONTENT_SETTINGS_TYPE_USB_CHOOSER_DATA:
      settings_type_string = "usb_chooser_data";
      break;
    case CONTENT_SETTINGS_TYPE_BLUETOOTH_GUARD:
      settings_type_string = "bluetooth_guard";
      break;
    case CONTENT_SETTINGS_TYPE_BACKGROUND_SYNC:
      settings_type_string = "background_sync";
      break;
    case CONTENT_SETTINGS_TYPE_AUTOPLAY:
      settings_type_string = "autoplay";
      break;
    // do nothing
    case CONTENT_SETTINGS_TYPE_DEFAULT:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_APP_BANNER:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_IMPORTANT_SITE_INFO:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_PERMISSION_AUTOBLOCKER_DATA:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_ADS:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_ADS_DATA:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_MIDI:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_PASSWORD_PROTECTION:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_MEDIA_ENGAGEMENT:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_SOUND:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_CLIENT_HINTS:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_SENSORS:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_ACCESSIBILITY_EVENTS:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_CLIPBOARD_READ:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_CLIPBOARD_WRITE:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_PLUGINS_DATA:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_PAYMENT_HANDLER:
      FALLTHROUGH;
    case CONTENT_SETTINGS_TYPE_USB_GUARD:
      FALLTHROUGH;
    case CONTENT_SETTINGS_NUM_TYPES:
      break;
  }
  DidBlockContentType(settings_type_string, base::UTF16ToUTF8(details));
}

void ContentSettingsObserver::DidBlockContentType(
    ContentSettingsType settings_type) {
  DidBlockContentType(settings_type, base::string16());
}

void ContentSettingsObserver::DidBlockContentType(
    const std::string& settings_type) {
  DidBlockContentType(settings_type,
      blink::WebStringToGURL(render_frame()->GetWebFrame()->
          GetSecurityOrigin().ToString()).spec());
}

void ContentSettingsObserver::DidBlockContentType(
    const std::string& settings_type,
    const std::string& details) {
  base::ListValue args;
  args.AppendString(settings_type);
  args.AppendString(details);

  auto rf = render_frame();
  rf->Send(new AtomViewHostMsg_Message(
    rf->GetRoutingID(), base::UTF8ToUTF16("content-blocked"), args));
}

bool ContentSettingsObserver::OnMessageReceived(const IPC::Message& message) {
  // Don't swallow LoadBlockedPlugins messages, as they're sent to every
  // blocked plugin.
  IPC_BEGIN_MESSAGE_MAP(ContentSettingsObserver, message)
    IPC_MESSAGE_HANDLER(ChromeViewMsg_LoadBlockedPlugins, OnLoadBlockedPlugins)
  IPC_END_MESSAGE_MAP()

  return false;
}

void ContentSettingsObserver::DidCommitProvisionalLoad(
    bool is_new_navigation,
    bool is_same_page_navigation) {
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->Parent())
    return;  // Not a top-level navigation.

  if (!is_same_page_navigation) {
    ClearBlockedContentSettings();
    temporarily_allowed_plugins_.clear();
  }
}

void ContentSettingsObserver::OnDestruct() {
  delete this;
}

bool ContentSettingsObserver::AllowDatabase(const WebString& name,
                                          const WebString& display_name,
                                          unsigned estimated_size) {  // NOLINT
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->GetSecurityOrigin().ToString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          ContentSettingsManager::GetOriginOrURL(frame),
          secondary_url,
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("database", secondary_url.spec());
  return allow;
}


void ContentSettingsObserver::RequestFileSystemAccessAsync(
        const WebContentSettingCallbacks& callbacks) {
  WebFrame* frame = render_frame()->GetWebFrame();
  WebContentSettingCallbacks permissionCallbacks(callbacks);
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique()) {
      permissionCallbacks.DoDeny();
      return;
  }

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->GetSecurityOrigin().ToString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          ContentSettingsManager::GetOriginOrURL(frame),
          secondary_url,
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }
  if (!allow) {
      DidBlockContentType("filesystem", secondary_url.spec());
      permissionCallbacks.DoDeny();
  } else {
      permissionCallbacks.DoAllow();
  }
}

bool ContentSettingsObserver::AllowImage(bool enabled_per_settings,
                                         const WebURL& image_url) {
  if (enabled_per_settings && IsWhitelistedForContentSettings())
    return true;

  bool allow = enabled_per_settings;
  GURL secondary_url(image_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
            ContentSettingsManager::GetOriginOrURL(
                render_frame()->GetWebFrame()),
            secondary_url,
            "images",
            allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("images", secondary_url.spec());
  return allow;
}

bool ContentSettingsObserver::AllowIndexedDB(const WebString& name,
                                             const WebSecurityOrigin& origin) {
  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;

  bool allow = true;
  GURL secondary_url(
      blink::WebStringToGURL(frame->GetSecurityOrigin().ToString()));
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
            ContentSettingsManager::GetOriginOrURL(frame),
            secondary_url,
            "cookies",
            allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("indexedDB", secondary_url.spec());
  return allow;
}

GURL GetOriginOrURL(const WebFrame* frame) {
  url::Origin top_origin = url::Origin(frame->Top()->GetSecurityOrigin());
  // The |top_origin| is unique ("null") e.g., for file:// URLs. Use the
  // document URL as the primary URL in those cases.
  // TODO(alexmos): This is broken for --site-per-process, since top() can be a
  // WebRemoteFrame which does not have a document(), and the WebRemoteFrame's
  // URL is not replicated.  See https://crbug.com/628759.
  if (top_origin.unique() && frame->Top()->IsWebLocalFrame())
    return frame->Top()->ToWebLocalFrame()->GetDocument().Url();
  return top_origin.GetURL();
}

// Allow passing both WebURL and GURL here, so that we can early return without
// allocating a new backing string if only the default rule matches.
template <typename URL>
ContentSetting GetContentSettingFromRules(
    const ContentSettingsForOneType& rules,
    const WebFrame* frame,
    const URL& secondary_url) {
  // If there is only one rule, it's the default rule and we don't need to match
  // the patterns.
  if (rules.size() == 1) {
    DCHECK(rules[0].primary_pattern == ContentSettingsPattern::Wildcard());
    DCHECK(rules[0].secondary_pattern == ContentSettingsPattern::Wildcard());
    return rules[0].GetContentSetting();
  }
  const GURL& primary_url = GetOriginOrURL(frame);
  const GURL& secondary_gurl = secondary_url;
  for (const auto& rule : rules) {
    if (rule.primary_pattern.Matches(primary_url) &&
        rule.secondary_pattern.Matches(secondary_gurl)) {
      return rule.GetContentSetting();
    }
  }
  NOTREACHED();
  return CONTENT_SETTING_DEFAULT;
}

bool IsScriptDisabledForPreview(const content::RenderFrame* render_frame) {
  return render_frame->GetPreviewsState() & content::NOSCRIPT_ON;
}

bool ContentSettingsObserver::AllowScript(bool enabled_per_settings) {
  if (!enabled_per_settings)
    return false;
  if (IsScriptDisabledForPreview(render_frame()))
    return false;
  if (is_interstitial_page_)
    return true;

  blink::WebLocalFrame* frame = render_frame()->GetWebFrame();
  const auto it = cached_script_permissions_.find(frame);
  if (it != cached_script_permissions_.end())
    return it->second;

  // Evaluate the content setting rules before
  // IsWhitelistedForContentSettings(); if there is only the default rule
  // allowing all scripts, it's quicker this way.
  bool allow = true;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          ContentSettingsManager::GetOriginOrURL(render_frame()->GetWebFrame()),
          url::Origin(frame->GetDocument().GetSecurityOrigin()).GetURL(),
          "javascript",
          allow) != CONTENT_SETTING_BLOCK;
  }
  allow = allow || IsWhitelistedForContentSettings();

  cached_script_permissions_[frame] = allow;
  return allow;
}

bool ContentSettingsObserver::AllowScriptFromSource(
    bool enabled_per_settings,
    const blink::WebURL& script_url) {
  if (IsWhitelistedForContentSettings())
    return true;

  bool allow = enabled_per_settings;
  GURL secondary_url(script_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          ContentSettingsManager::GetOriginOrURL(render_frame()->GetWebFrame()),
          secondary_url,
          "javascript",
          allow) != CONTENT_SETTING_BLOCK;
  }

  allow = allow || IsWhitelistedForContentSettings();
  if (!allow)
    DidBlockContentType("javascript", secondary_url.spec());
  return allow;
}

bool ContentSettingsObserver::AllowStorage(bool local) {
  if (IsWhitelistedForContentSettings())
    return true;

  WebFrame* frame = render_frame()->GetWebFrame();
  if (frame->GetSecurityOrigin().IsUnique() ||
      frame->Top()->GetSecurityOrigin().IsUnique())
    return false;

  StoragePermissionsKey key(
      blink::WebStringToGURL(frame->ToWebLocalFrame()->GetDocument().GetSecurityOrigin().ToString()),
      local);
  std::map<StoragePermissionsKey, bool>::const_iterator permissions =
      cached_storage_permissions_.find(key);
  if (permissions != cached_storage_permissions_.end())
    return permissions->second;

  bool allow = true;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
          ContentSettingsManager::GetOriginOrURL(frame),
          blink::WebStringToGURL(frame->GetSecurityOrigin().ToString()),
          "cookies",
          allow) != CONTENT_SETTING_BLOCK;
  }

  cached_storage_permissions_[key] = allow;
  if (!allow)
    DidBlockContentType("storage");
  return allow;
}

bool ContentSettingsObserver::AllowReadFromClipboard(bool default_value) {
  bool allowed = default_value;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ScriptContext* current_context =
      extension_dispatcher_->script_context_set().GetCurrent();
  if (current_context) {
    allowed |= current_context->HasAPIPermission(
        extensions::APIPermission::kClipboardRead);
  }
#endif
  return allowed;
}

bool ContentSettingsObserver::AllowWriteToClipboard(bool default_value) {
  bool allowed = default_value;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // All blessed extension pages could historically write to the clipboard, so
  // preserve that for compatibility.
  extensions::ScriptContext* current_context =
      extension_dispatcher_->script_context_set().GetCurrent();
  if (current_context) {
    if (current_context->effective_context_type() ==
        extensions::Feature::BLESSED_EXTENSION_CONTEXT) {
      allowed = true;
    } else {
      allowed |= current_context->HasAPIPermission(
          extensions::APIPermission::kClipboardWrite);
    }
  }
#endif
  return allowed;
}

bool ContentSettingsObserver::AllowMutationEvents(bool default_value) {
  if (IsWhitelistedForContentSettings())
    return true;

  bool allow = default_value;
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
            ContentSettingsManager::GetOriginOrURL(
                render_frame()->GetWebFrame()),
            GURL(),
            "mutation",
            allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType("mutation");
  return allow;
}

bool ContentSettingsObserver::AllowRunningInsecureContent(
    bool allowed_per_settings,
    const blink::WebSecurityOrigin& origin,
    const blink::WebURL& resource_url) {
  // TODO(bridiver) is origin different than web frame top origin?
  bool allow = allowed_per_settings;
  GURL secondary_url(resource_url);
  if (content_settings_manager_->content_settings()) {
    allow =
        content_settings_manager_->GetSetting(
            ContentSettingsManager::GetOriginOrURL(
                render_frame()->GetWebFrame()),
            secondary_url,
            "runInsecureContent",
            allow) != CONTENT_SETTING_BLOCK;
  }

  if (allow)
    DidRunInsecureContent(GURL(resource_url));
  else
    DidBlockRunInsecureContent(GURL(resource_url));
  return allow;
}

bool ContentSettingsObserver::AllowAutoplay(bool default_value) {
  bool allow = default_value;
  if (content_settings_manager_->content_settings()) {
    WebFrame* frame = render_frame()->GetWebFrame();
    auto origin = frame->ToWebLocalFrame()->GetDocument().GetSecurityOrigin();
    allow =
        content_settings_manager_->GetSetting(
            ContentSettingsManager::GetOriginOrURL(frame),
            blink::WebStringToGURL(origin.ToString()),
            "autoplay",
            allow) != CONTENT_SETTING_BLOCK;
  }

  if (!allow)
    DidBlockContentType(CONTENT_SETTINGS_TYPE_AUTOPLAY);
  return allow;
}

void ContentSettingsObserver::DidNotAllowPlugins() {
  DidBlockContentType(CONTENT_SETTINGS_TYPE_PLUGINS);
}

void ContentSettingsObserver::DidNotAllowScript() {
  DidBlockContentType(CONTENT_SETTINGS_TYPE_JAVASCRIPT);
}

void ContentSettingsObserver::OnLoadBlockedPlugins(
    const std::string& identifier) {
  temporarily_allowed_plugins_.insert(identifier);
}

void ContentSettingsObserver::DidRunInsecureContent(GURL resouce_url) {
  base::ListValue args;
    args.AppendString(resouce_url.spec());

    auto rf = render_frame();
    rf->Send(new AtomViewHostMsg_Message(rf->GetRoutingID(),
        base::UTF8ToUTF16("did-run-insecure-content"), args));
}

void ContentSettingsObserver::DidBlockRunInsecureContent(GURL resouce_url) {
  base::ListValue args;
    args.AppendString(resouce_url.spec());

    auto rf = render_frame();
    rf->Send(new AtomViewHostMsg_Message(rf->GetRoutingID(),
        base::UTF8ToUTF16("did-block-run-insecure-content"), args));
}

void ContentSettingsObserver::ClearBlockedContentSettings() {
  cached_storage_permissions_.clear();
  cached_script_permissions_.clear();
}

bool ContentSettingsObserver::IsWhitelistedForContentSettings() const {
  // Whitelist ftp directory listings, as they require JavaScript to function
  // properly.
  if (!render_frame()) {
    return false;
  }

  if (render_frame()->IsFTPDirectoryListing())
    return true;

  WebFrame* web_frame = render_frame()->GetWebFrame();

  if (!web_frame) {
    return false;
  }

  return IsWhitelistedForContentSettings(
      web_frame->ToWebLocalFrame()->GetDocument().GetSecurityOrigin(),
      web_frame->ToWebLocalFrame()->GetDocument().Url());
}

bool ContentSettingsObserver::IsWhitelistedForContentSettings(
    const WebSecurityOrigin& origin,
    const GURL& document_url) {
  if (document_url == GURL(content::kUnreachableWebDataURL))
    return true;

  if (origin.IsUnique())
    return false;  // Uninitialized document?

  base::string16 protocol = origin.Protocol().Utf16();
  if (base::EqualsASCII(protocol, content::kChromeUIScheme))
    return true;  // Browser UI elements should still work.

  if (base::EqualsASCII(protocol, content::kChromeDevToolsScheme))
    return true;  // DevTools UI elements should still work.

#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (base::EqualsASCII(protocol, extensions::kExtensionScheme))
    return true;
#endif

  // If the scheme is file:, an empty file name indicates a directory listing,
  // which requires JavaScript to function properly.
  if (base::EqualsASCII(protocol, url::kFileScheme)) {
    return document_url.SchemeIs(url::kFileScheme) &&
           document_url.ExtractFileName().empty();
  }

  return false;
}
