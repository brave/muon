// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_contents_preferences.h"

#include <algorithm>
#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_number_conversions.h"
#include "cc/base/switches.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/web_preferences.h"
#include "native_mate/dictionary.h"
#include "net/base/filename_util.h"

#if defined(OS_WIN)
#include "ui/gfx/switches.h"
#endif

DEFINE_WEB_CONTENTS_USER_DATA_KEY(atom::WebContentsPreferences);

namespace atom {

// static
std::vector<WebContentsPreferences*> WebContentsPreferences::instances_;

WebContentsPreferences::WebContentsPreferences(
    content::WebContents* web_contents,
    const mate::Dictionary& web_preferences)
    : web_contents_(web_contents) {
  v8::Isolate* isolate = web_preferences.isolate();
  mate::Dictionary copied(isolate, web_preferences.GetHandle()->Clone());
  // Following fields should not be stored.
  copied.Delete("embedder");
  copied.Delete("isGuest");
  copied.Delete("session");

  mate::ConvertFromV8(isolate, copied.GetHandle(), &web_preferences_);
  web_contents->SetUserData(UserDataKey(), this);

  instances_.push_back(this);
}

WebContentsPreferences::~WebContentsPreferences() {
  instances_.erase(
      std::remove(instances_.begin(), instances_.end(), this),
      instances_.end());
}

void WebContentsPreferences::Merge(const base::DictionaryValue& extend) {
  web_preferences_.MergeDictionary(&extend);
}

// static
content::WebContents* WebContentsPreferences::GetWebContentsFromProcessID(
    int process_id) {
  for (WebContentsPreferences* preferences : instances_) {
    content::WebContents* web_contents = preferences->web_contents_;
    if (web_contents->GetRenderProcessHost()->GetID() == process_id)
      return web_contents;
  }
  // Also try to get the webview from RenderViewHost::FromID because
  // not all web contents have preferences created (devtools).
  content::WebContents* web_contents = nullptr;
  // The default routing id of WebContents.
  int kDefaultRoutingID = 1;
  auto rvh = content::RenderViewHost::FromID(process_id, kDefaultRoutingID);
  if (rvh) {
    web_contents = content::WebContents::FromRenderViewHost(rvh);
  }
  return web_contents;
}

// static
void WebContentsPreferences::AppendExtraCommandLineSwitches(
    content::WebContents* web_contents, base::CommandLine* command_line) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  base::DictionaryValue& web_preferences = self->web_preferences_;

  bool b;

  // Experimental flags.
  if (web_preferences.GetBoolean(options::kExperimentalFeatures, &b) && b)
    command_line->AppendSwitch(
        ::switches::kEnableExperimentalWebPlatformFeatures);
  if (web_preferences.GetBoolean(options::kExperimentalCanvasFeatures, &b) && b)
    command_line->AppendSwitch(::switches::kEnableExperimentalCanvasFeatures);

  // --background-color.
  std::string color;
  if (web_preferences.GetString(options::kBackgroundColor, &color))
    command_line->AppendSwitchASCII(switches::kBackgroundColor, color);

  // The zoom factor.
  double zoom_factor = 1.0;
  if (web_preferences.GetDouble(options::kZoomFactor, &zoom_factor) &&
      zoom_factor != 1.0)
    command_line->AppendSwitchASCII(switches::kZoomFactor,
                                    base::DoubleToString(zoom_factor));

  // Custom command line switches.
  const base::ListValue* args;
  if (web_preferences.GetList("commandLineSwitches", &args)) {
    for (size_t i = 0; i < args->GetSize(); ++i) {
      std::string arg;
      if (args->GetString(i, &arg) && !arg.empty())
        command_line->AppendSwitch(arg);
    }
  }

  // Enable blink features.
  std::string blink_features;
  if (web_preferences.GetString(options::kBlinkFeatures, &blink_features))
    command_line->AppendSwitchASCII(::switches::kEnableBlinkFeatures,
                                    blink_features);

  // Disable blink features.
  std::string disable_blink_features;
  if (web_preferences.GetString(options::kDisableBlinkFeatures,
                                &disable_blink_features))
    command_line->AppendSwitchASCII(::switches::kDisableBlinkFeatures,
                                    disable_blink_features);
}

// static
void WebContentsPreferences::OverrideWebkitPrefs(
    content::WebContents* web_contents, content::WebPreferences* prefs) {
  WebContentsPreferences* self = FromWebContents(web_contents);
  if (!self)
    return;

  bool b;
  if (self->web_preferences_.GetBoolean("javascript", &b))
    prefs->javascript_enabled = b;
  if (self->web_preferences_.GetBoolean("images", &b))
    prefs->images_enabled = b;
  if (self->web_preferences_.GetBoolean("domPasteEnabled", &b))
    prefs->dom_paste_enabled = b;
  if (self->web_preferences_.GetBoolean("javascriptCanAccessClipboard", &b))
    prefs->javascript_can_access_clipboard = b;
  if (self->web_preferences_.GetBoolean("textAreasAreResizable", &b))
    prefs->text_areas_are_resizable = b;
  if (self->web_preferences_.GetBoolean("webgl", &b))
    prefs->experimental_webgl_enabled = b;
  if (self->web_preferences_.GetBoolean("webSecurity", &b))
    prefs->web_security_enabled = b;
  if (self->web_preferences_.GetBoolean("allowFileAccessFromFileUrls", &b))
    prefs->allow_file_access_from_file_urls = b;
  if (self->web_preferences_.GetBoolean("allowUniversalAccessFromFileUrls", &b))
    prefs->allow_universal_access_from_file_urls = b;
  if (self->web_preferences_.GetBoolean("allowRunningInsecureContent", &b))
    prefs->allow_running_insecure_content = b;
  if (self->web_preferences_.GetBoolean("hyperlinkAuditingEnabled", &b))
    prefs->hyperlink_auditing_enabled = b;
  if (self->web_preferences_.GetBoolean("touchEnabled", &b))
    prefs->touch_event_feature_detection_enabled = b;
  const base::DictionaryValue* fonts = nullptr;
  if (self->web_preferences_.GetDictionary("defaultFontFamily", &fonts)) {
    base::string16 font;
    if (fonts->GetString("standard", &font))
      prefs->standard_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("serif", &font))
      prefs->serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("sansSerif", &font))
      prefs->sans_serif_font_family_map[content::kCommonScript] = font;
    if (fonts->GetString("monospace", &font))
      prefs->fixed_font_family_map[content::kCommonScript] = font;
  }
  int size;
  if (self->web_preferences_.GetInteger("defaultFontSize", &size))
    prefs->default_font_size = size;
  if (self->web_preferences_.GetInteger("defaultMonospaceFontSize", &size))
    prefs->default_fixed_font_size = size;
  if (self->web_preferences_.GetInteger("minimumFontSize", &size))
    prefs->minimum_font_size = size;
  std::string encoding;
  if (self->web_preferences_.GetString("defaultEncoding", &encoding))
    prefs->default_encoding = encoding;
}

}  // namespace atom
