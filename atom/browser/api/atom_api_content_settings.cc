// Copyright (c) 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_content_settings.h"

#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/v8_value_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/values.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "native_mate/object_template_builder.h"

namespace mate {

template<>
struct Converter<ContentSettingsPattern> {
  static bool FromV8(v8::Isolate* isolate,
      v8::Local<v8::Value> val, ContentSettingsPattern* out) {
    if (val->IsNull() || val->IsUndefined()) {
      *out = ContentSettingsPattern::Wildcard();
      return true;
    }

    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;

    *out = ContentSettingsPattern::FromString(pattern);
    return true;
  }
};

template<>
struct Converter<ContentSettingsType> {
  static bool FromV8(v8::Isolate* isolate,
      v8::Local<v8::Value> val, ContentSettingsType* out) {
    std::string settings_type_string;
    if (!ConvertFromV8(isolate, val, &settings_type_string))
      return false;

    if (settings_type_string == "cookies")
      *out = CONTENT_SETTINGS_TYPE_COOKIES;
    else if (settings_type_string == "images")
      *out = CONTENT_SETTINGS_TYPE_IMAGES;
    else if (settings_type_string == "javascript")
      *out = CONTENT_SETTINGS_TYPE_JAVASCRIPT;
    else if (settings_type_string == "plugins")
      *out = CONTENT_SETTINGS_TYPE_PLUGINS;
    else if (settings_type_string == "popups")
      *out = CONTENT_SETTINGS_TYPE_POPUPS;
    else if (settings_type_string == "geo")
      *out = CONTENT_SETTINGS_TYPE_GEOLOCATION;
    else if (settings_type_string == "notifications")
      *out = CONTENT_SETTINGS_TYPE_NOTIFICATIONS;
    else if (settings_type_string == "auto_select_certificate")
      *out = CONTENT_SETTINGS_TYPE_AUTO_SELECT_CERTIFICATE;
    else if (settings_type_string == "runInsecureCon {tent")
      *out = CONTENT_SETTINGS_TYPE_MIXEDSCRIPT;
    else if (settings_type_string == "mediastream_mic")
      *out = CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC;
    else if (settings_type_string == "mediastream_camera")
      *out = CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA;
    else if (settings_type_string == "protocol_handlers")
      *out = CONTENT_SETTINGS_TYPE_PROTOCOL_HANDLERS;
    else if (settings_type_string == "ppapi_broker")
      *out = CONTENT_SETTINGS_TYPE_PPAPI_BROKER;
    else if (settings_type_string == "automatic_downloads")
      *out = CONTENT_SETTINGS_TYPE_AUTOMATIC_DOWNLOADS;
    else if (settings_type_string == "midi_sysex")
      *out = CONTENT_SETTINGS_TYPE_MIDI_SYSEX;
    else if (settings_type_string == "ssl_cert_decisions")
      *out = CONTENT_SETTINGS_TYPE_SSL_CERT_DECISIONS;
    else if (settings_type_string == "protected_media_identifiers")
      *out = CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER;
    else if (settings_type_string == "site_engagement")
      *out = CONTENT_SETTINGS_TYPE_SITE_ENGAGEMENT;
    else if (settings_type_string == "durable_storage")
      *out = CONTENT_SETTINGS_TYPE_DURABLE_STORAGE;
    else if (settings_type_string == "usb_chooser_data")
      *out = CONTENT_SETTINGS_TYPE_USB_CHOOSER_DATA;
    else if (settings_type_string == "bluetooth_guard")
      *out = CONTENT_SETTINGS_TYPE_BLUETOOTH_GUARD;
    else if (settings_type_string == "background_sync")
      *out = CONTENT_SETTINGS_TYPE_BACKGROUND_SYNC;
    else if (settings_type_string == "autoplay")
      *out = CONTENT_SETTINGS_TYPE_AUTOPLAY;
    else
      return false;

    return true;
  }
};


template<>
struct Converter<ContentSetting> {
  static bool FromV8(v8::Isolate* isolate,
      v8::Local<v8::Value> val, ContentSetting* out) {
    std::string setting_string;
    if (!ConvertFromV8(isolate, val, &setting_string))
      return false;

    if (setting_string == "default")
      *out = CONTENT_SETTING_DEFAULT;
    else if (setting_string == "allow")
      *out = CONTENT_SETTING_ALLOW;
    else if (setting_string == "block")
      *out = CONTENT_SETTING_BLOCK;
    else if (setting_string == "deny")
      *out = CONTENT_SETTING_BLOCK;
    else if (setting_string == "session")
      *out = CONTENT_SETTING_SESSION_ONLY;
    else if (setting_string == "important")
      *out = CONTENT_SETTING_DETECT_IMPORTANT_CONTENT;
    else
      return false;

    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

ContentSettings::ContentSettings(v8::Isolate* isolate,
                 content::BrowserContext* browser_context)
      : browser_context_(browser_context) {
  Init(isolate);
}

ContentSettings::~ContentSettings() {
}

Profile* ContentSettings::profile() {
  return Profile::FromBrowserContext(browser_context_);
}

bool ContentSettings::SetDefaultContentSetting(
    ContentSettingsType content_type,
    ContentSetting setting) {
  auto map = HostContentSettingsMapFactory::GetForProfile(profile());

  if (!map)
    return false;

  map->SetDefaultContentSetting(content_type, setting);
  return true;
}

bool ContentSettings::ClearForOneType(ContentSettingsType content_type) {
  auto map = HostContentSettingsMapFactory::GetForProfile(profile());

  if (!map)
    return false;

  map->ClearSettingsForOneType(content_type);
  return true;
}

bool ContentSettings::SetContentSetting(mate::Arguments* args) {
  if (args->Length() != 5) {
    args->ThrowError("Wrong number of arguments");
  }

  ContentSettingsPattern primary;
  if (!args->GetNext(&primary)) {
    args->ThrowError("primaryPattern is a required field");
    return false;
  }

  ContentSettingsPattern secondary;
  if (!args->GetNext(&secondary)) {
    secondary = ContentSettingsPattern::Wildcard();
  }

  ContentSettingsType content_type;
  if (!args->GetNext(&content_type)) {
    args->ThrowError("contentType is a required field");
    return false;
  }

  std::string resource_identifier;
  if (!args->GetNext(&resource_identifier)) {
    resource_identifier = "";
  }

  ContentSetting setting;
  if (!args->GetNext(&setting)) {
    args->ThrowError("setting is a required field");
    return false;
  }

  auto map = HostContentSettingsMapFactory::GetForProfile(profile());

  if (!map)
    return false;

  map->SetContentSettingCustomScope(primary, secondary, content_type,
      resource_identifier, setting);

  return true;
}

// static
mate::Handle<ContentSettings> ContentSettings::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  DCHECK(browser_context);
  return mate::CreateHandle(isolate,
      new ContentSettings(isolate, browser_context));
}

// static
void ContentSettings::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "ContentSettings"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("setDefault", &ContentSettings::SetDefaultContentSetting)
      .SetMethod("clearForOneType", &ContentSettings::ClearForOneType)
      .SetMethod("set", &ContentSettings::SetContentSetting);
}

}  // namespace api

}  // namespace atom
