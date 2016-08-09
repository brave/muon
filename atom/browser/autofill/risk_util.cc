// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/autofill/risk_util.h"

#include <memory>

#include "atom/common/chrome_version.h"
#include "base/base64.h"
#include "base/callback.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/autofill/content/browser/risk/fingerprint.h"
#include "components/autofill/content/browser/risk/proto/fingerprint.pb.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#include "ui/base/base_window.h"

namespace autofill {

namespace {

void PassRiskData(const base::Callback<void(const std::string&)>& callback,
                  std::unique_ptr<risk::Fingerprint> fingerprint) {
  std::string proto_data, risk_data;
  fingerprint->SerializeToString(&proto_data);
  base::Base64Encode(proto_data, &risk_data);
  callback.Run(risk_data);
}

}  // namespace

void LoadRiskData(uint64_t obfuscated_gaia_id,
                  content::WebContents* web_contents,
                  const base::Callback<void(const std::string&)>& callback) {
  gfx::Rect window_bounds = web_contents->GetContainerBounds();
/*
  const PrefService* user_prefs =
      Profile::FromBrowserContext(web_contents->GetBrowserContext())
          ->GetPrefs();
  std::string charset = user_prefs->GetString(::prefs::kDefaultCharset);
  std::string accept_languages =
      user_prefs->GetString(::prefs::kAcceptLanguages);
      */
  // TODO(anthony): fill in these values
  std::string charset;
  std::string accept_languages;
  base::Time install_time;
  /*
  base::Time install_time = base::Time::FromTimeT(
      g_browser_process->metrics_service()->GetInstallDate());
      */

  risk::GetFingerprint(obfuscated_gaia_id, window_bounds, web_contents,
                       CHROME_VERSION_STRING, charset,
                       accept_languages, install_time,
                       g_browser_process->GetApplicationLocale(),
                       web_contents->GetUserAgentOverride(),
                       base::Bind(PassRiskData, callback));
}

}  // namespace autofill
