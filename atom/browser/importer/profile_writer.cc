// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/importer/profile_writer.h"

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_app.h"
#include "atom/browser/api/atom_api_importer.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/base64.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/common/importer/imported_cookie_entry.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/password_manager/password_store_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/importer/imported_bookmark_entry.h"
#include "components/autofill/core/browser/webdata/autofill_entry.h"
#include "components/autofill/core/common/password_form.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "content/public/browser/browser_thread.h"

#if defined(OS_WIN)
#include "components/password_manager/core/browser/webdata/password_web_data_service_win.h"
#endif

namespace importer {
void ShowImportLockDialog(gfx::NativeWindow parent,
                          const base::Callback<void(bool)>& callback) {
  atom::api::App *app =
    static_cast<BrowserProcessImpl*>(g_browser_process)->app();
  if (app) {
    app->Emit("show-warning-dialog");
  }
  // TODO(darkdh): emit callback and let users have continue option without
  // restart anotehr import process
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE, base::Bind(callback, false));
}
}  // namespace importer

namespace atom {

ProfileWriter::ProfileWriter(Profile* profile) :
    ::ProfileWriter(profile),
    importer_(nullptr) {}

bool ProfileWriter::BookmarkModelIsLoaded() const {
  return true;
}

bool ProfileWriter::TemplateURLServiceIsLoaded() const {
  return true;
}

void ProfileWriter::AddPasswordForm(const autofill::PasswordForm& form) {
    Profile* active_profile = ProfileManager::GetActiveUserProfile();
    if (active_profile) {
      PasswordStoreFactory::GetForProfile(
          active_profile, ServiceAccessType::EXPLICIT_ACCESS)->AddLogin(form);
    }
}

#if defined(OS_WIN)
void ProfileWriter::AddIE7PasswordInfo(const IE7PasswordInfo& info) {
}
#endif

void ProfileWriter::AddHistoryPage(const history::URLRows& page,
                                   history::VisitSource visit_source) {
  if (importer_) {
    base::ListValue history_list;
    for (const history::URLRow& row : page) {
      base::DictionaryValue* history = new base::DictionaryValue();
      history->SetString("title", row.title());
      history->SetString("url", row.url().possibly_invalid_spec());
      history->SetInteger("visit_count", row.visit_count());
      history->SetInteger("last_visit", row.last_visit().ToDoubleT());
      history_list.Append(std::unique_ptr<base::DictionaryValue>(history));
    }
    importer_->Emit("add-history-page", history_list,
                    (unsigned int) visit_source);
  }
}

void ProfileWriter::AddHomepage(const GURL& home_page) {
  if (importer_) {
    importer_->Emit("add-homepage", home_page.possibly_invalid_spec());
  }
}

void ProfileWriter::AddBookmarks(
    const std::vector<ImportedBookmarkEntry>& bookmarks,
    const base::string16& top_level_folder_name) {
  if (bookmarks.empty())
    return;

  if (importer_) {
    base::ListValue imported_bookmarks;
    for (const ImportedBookmarkEntry& bookmark : bookmarks) {
      base::DictionaryValue* imported_bookmark = new base::DictionaryValue();
      imported_bookmark->SetBoolean("in_toolbar", bookmark.in_toolbar);
      imported_bookmark->SetBoolean("is_folder", bookmark.is_folder);
      imported_bookmark->SetString("url", bookmark.url.possibly_invalid_spec());
      imported_bookmark->SetString("title", bookmark.title);
      imported_bookmark->SetInteger("creation_time",
                                    bookmark.creation_time.ToDoubleT());
      auto paths = base::MakeUnique<base::ListValue>();
      for (const base::string16& path : bookmark.path) {
        paths->AppendString(path);
      }
      imported_bookmark->Set("path", std::move(paths));
      imported_bookmarks.Append(std::unique_ptr<base::DictionaryValue>(
                                    imported_bookmark));
    }
    importer_->Emit("add-bookmarks", imported_bookmarks, top_level_folder_name);
  }
}

void ProfileWriter::AddFavicons(
    const favicon_base::FaviconUsageDataList& favicons) {
  if (importer_) {
    base::ListValue imported_favicons;
    for (const favicon_base::FaviconUsageData& favicon : favicons) {
      base::DictionaryValue* imported_favicon = new base::DictionaryValue();
      imported_favicon->SetString("favicon_url",
                                  favicon.favicon_url.possibly_invalid_spec());
      std::string data_url;
      data_url.insert(data_url.end(), favicon.png_data.begin(),
                      favicon.png_data.end());
      base::Base64Encode(data_url, &data_url);
      data_url.insert(0, "data:image/png;base64,");
      imported_favicon->SetString("png_data", data_url);
      std::set<GURL>::iterator it;
      auto urls = base::MakeUnique<base::ListValue>();
      for (it = favicon.urls.begin(); it != favicon.urls.end(); ++it) {
        urls->AppendString(it->possibly_invalid_spec());
      }
      imported_favicon->Set("urls", std::move(urls));
      imported_favicons.Append(std::unique_ptr<base::DictionaryValue>(
                                  imported_favicon));
    }
    importer_->Emit("add-favicons", imported_favicons);
  }
}


void ProfileWriter::AddAutofillFormDataEntries(
    const std::vector<autofill::AutofillEntry>& autofill_entries) {
  if (importer_) {
    base::ListValue imported_autofill_entries;
    for (const autofill::AutofillEntry& autofill_entry : autofill_entries) {
      base::DictionaryValue* imported_autofill_entry =
        new base::DictionaryValue();
      imported_autofill_entry->SetString("name", autofill_entry.key().name());
      imported_autofill_entry->SetString("value", autofill_entry.key().value());
      imported_autofill_entries.Append(std::unique_ptr<base::DictionaryValue>(
                                          imported_autofill_entry));
    }
    importer_->Emit("add-autofill-form-data-entries",
                    imported_autofill_entries);
  }
}

void ProfileWriter::AddCookies(
    const std::vector<ImportedCookieEntry>& cookies) {
  if (importer_) {
    base::ListValue imported_cookies;
    for (const ImportedCookieEntry& cookie_entry : cookies) {
      base::DictionaryValue* cookie = new base::DictionaryValue();
      base::string16 url;
      if (cookie_entry.httponly) {
        url.append(base::UTF8ToUTF16("http://"));
        url.append(cookie_entry.host);
      } else {
        url.append(base::UTF8ToUTF16("https://"));
        url.append(cookie_entry.host);
      }
      cookie->SetString("url", url);
      cookie->SetString("domain", cookie_entry.domain);
      cookie->SetString("name", cookie_entry.name);
      cookie->SetString("value", cookie_entry.value);
      cookie->SetString("path", cookie_entry.path);
      cookie->SetInteger("expiry_date", cookie_entry.expiry_date.ToDoubleT());
      cookie->SetBoolean("secure", cookie_entry.secure);
      cookie->SetBoolean("httponly", cookie_entry.httponly);
      imported_cookies.Append(std::unique_ptr<base::DictionaryValue>(cookie));
    }
    importer_->Emit("add-cookies", imported_cookies);
  }
}

void ProfileWriter::Initialize(atom::api::Importer* importer) {
  importer_ = importer;
}

ProfileWriter::~ProfileWriter() {}

}  // namespace atom
