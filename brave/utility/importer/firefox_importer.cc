// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/utility/importer/firefox_importer.h"

#include <vector>

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/macros.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/imported_cookie_entry.h"
#include "brave/utility/importer/brave_external_process_importer_bridge.h"
#include "build/build_config.h"
#include "chrome/grit/generated_resources.h"
#include "components/autofill/core/common/password_form.h"
#include "sql/connection.h"
#include "sql/statement.h"
#include "url/gurl.h"

namespace brave {

FirefoxImporter::FirefoxImporter() {
}

FirefoxImporter::~FirefoxImporter() {
}

void FirefoxImporter::StartImport(const importer::SourceProfile& source_profile,
                                  uint16_t items,
                                  ImporterBridge* bridge) {
  ::FirefoxImporter::StartImport(source_profile, items, bridge);

  bridge_ = bridge;
  source_path_ = source_profile.source_path;
  if ((items & importer::COOKIES) && !cancelled()) {
    bridge_->NotifyItemStarted(importer::COOKIES);
    ImportCookies();
    bridge_->NotifyItemEnded(importer::COOKIES);
  }
  if ((items & importer::PASSWORDS) && !cancelled()) {
    ImportSitePasswordPrefs();
    bridge_->NotifyItemEnded(importer::PASSWORDS);
  }

  bridge_->NotifyEnded();
}


void FirefoxImporter::ImportCookies() {
  base::FilePath file = source_path_.AppendASCII("cookies.sqlite");
  if (!base::PathExists(file)) {
    return;
  }

  sql::Connection db;
  if (!db.Open(file)) {
    return;
  }

  const char query[] =
      "SELECT baseDomain, name, value, host, path, expiry, isSecure, "
      "isHttpOnly FROM moz_cookies";

  sql::Statement s(db.GetUniqueStatement(query));

  std::vector<ImportedCookieEntry> cookies;
  while (s.Step() && !cancelled()) {
    ImportedCookieEntry cookie;
    base::string16 domain(base::UTF8ToUTF16("."));
    domain.append(s.ColumnString16(0));
    base::string16 host;
    if (s.ColumnString16(3)[0] == '.') {
      host.append(base::UTF8ToUTF16("*"));
      host.append(s.ColumnString16(3));
    } else {
      host = s.ColumnString16(3);
    }
    cookie.domain = domain;
    cookie.name = s.ColumnString16(1);
    cookie.value = s.ColumnString16(2);
    cookie.host = host;
    cookie.path = s.ColumnString16(4);
    cookie.expiry_date =
      base::Time::FromDoubleT(s.ColumnInt64(5));
    cookie.secure = s.ColumnBool(6);
    cookie.httponly = s.ColumnBool(7);

    cookies.push_back(cookie);
  }

  if (!cookies.empty() && !cancelled())
    static_cast<BraveExternalProcessImporterBridge*>(bridge_.get())->
        SetCookies(cookies);
}

void FirefoxImporter::ImportSitePasswordPrefs() {
  base::FilePath file = source_path_.AppendASCII("permissions.sqlite");
  if (!base::PathExists(file)) {
    return;
  }

  sql::Connection db;
  if (!db.Open(file)) {
    return;
  }

  // See http://searchfox.org/mozilla-central/
  // source/netwerk/base/nsIPermissionManager.idl#61
  // DENY_ACTION=2
  // EXPIRE_NEVER=0
  const char query[] =
      "SELECT origin FROM moz_perms "
      "WHERE type='login-saving' AND permission=2 AND expireType=0";


  std::vector<autofill::PasswordForm> forms;
  sql::Statement s(db.GetUniqueStatement(query));

  while (s.Step() && !cancelled()) {
    autofill::PasswordForm form;
    form.origin = GURL(s.ColumnString16(0));
    form.signon_realm = form.origin.GetOrigin().spec();
    form.blacklisted_by_user = true;

    forms.push_back(form);
  }

  if (!cancelled()) {
    for (size_t i = 0; i < forms.size(); ++i) {
        bridge_->SetPasswordForm(forms[i]);
    }
  }
}

}  // namespace brave
