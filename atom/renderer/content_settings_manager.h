// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_
#define ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_

#include <memory>
#include <string>
#include <vector>
#include "base/values.h"
#include "base/lazy_instance.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/common/web_preferences.h"
#include "content/public/renderer/render_thread_observer.h"


class GURL;

namespace base {
class DictionaryValue;
}

namespace atom {

class ContentSettingsManager : public content::RenderThreadObserver {
 public:
  ContentSettingsManager();
  ~ContentSettingsManager() override;

  static ContentSettingsManager* GetInstance();

  const base::DictionaryValue* content_settings() const
    { return content_settings_.get(); };

  ContentSetting GetSetting(
      GURL primary_url,
      GURL secondary_url,
      std::string resource_id,
      bool incognito);

  std::vector<std::string> GetContentTypes();

 private:
  ContentSetting GetContentSettingFromRules(
    const GURL& primary_url,
    const GURL& secondary_url,
    const std::string& content_type,
    const bool& enabled_per_settings);

  // content::RenderThreadObserver:
  bool OnControlMessageReceived(const IPC::Message& message) override;

  void OnUpdateWebKitPrefs(
      const content::WebPreferences& web_preferences);
  void OnUpdateContentSettings(
      const base::DictionaryValue& content_settings);


  content::WebPreferences web_preferences_;
  std::unique_ptr<base::DictionaryValue> content_settings_;

  DISALLOW_COPY_AND_ASSIGN(ContentSettingsManager);
};

}  // namespace atom

#endif  // ATOM_RENDERER_CONTENT_SETTINGS_MANAGER_H_
