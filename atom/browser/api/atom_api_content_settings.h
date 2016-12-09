// Copyright (c) 2016 Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_CONTENT_SETTINGS_H_
#define ATOM_BROWSER_API_ATOM_API_CONTENT_SETTINGS_H_

#include <string>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "brave/browser/brave_browser_context.h"
#include "components/content_settings/core/common/content_settings.h"
#include "native_mate/handle.h"

class ContentSettingPattern;
class Profile;

namespace atom {

namespace api {

class ContentSettings : public mate::TrackableObject<ContentSettings> {
 public:
  static mate::Handle<ContentSettings> Create(v8::Isolate* isolate,
                                  content::BrowserContext* browser_context);

  // mate::TrackableObject:
  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  ContentSettings(v8::Isolate* isolate, content::BrowserContext* browser_context);
  ~ContentSettings() override;

  bool ClearForOneType(ContentSettingsType content_type);

  bool SetDefaultContentSetting(ContentSettingsType content_type,
      ContentSetting setting);

  bool SetContentSetting(mate::Arguments* args);

  Profile* profile();
 private:
  content::BrowserContext* browser_context_;  // not owned

  DISALLOW_COPY_AND_ASSIGN(ContentSettings);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_CONTENT_SETTINGS_H_
