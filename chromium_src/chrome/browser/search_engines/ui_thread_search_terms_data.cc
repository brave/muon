// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search_engines/ui_thread_search_terms_data.h"

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"


using content::BrowserThread;

// static
std::string* UIThreadSearchTermsData::google_base_url_ = NULL;

UIThreadSearchTermsData::UIThreadSearchTermsData(Profile* profile)
    : profile_(profile) {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
}

std::string UIThreadSearchTermsData::GoogleBaseURLValue() const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  if (google_base_url_)
    return *google_base_url_;

  if (!profile_)
    return std::string();

  return std::string();
}

std::string UIThreadSearchTermsData::GetApplicationLocale() const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  return g_browser_process->GetApplicationLocale();
}

base::string16 UIThreadSearchTermsData::GetRlzParameterValue(
    bool from_app_list) const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  return base::string16();
}

std::string UIThreadSearchTermsData::GetSearchClient() const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  return std::string();
}

std::string UIThreadSearchTermsData::GetSuggestClient() const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  return std::string();
}

std::string UIThreadSearchTermsData::GetSuggestRequestIdentifier() const {
  DCHECK(!BrowserThread::IsThreadInitialized(BrowserThread::UI) ||
      BrowserThread::CurrentlyOn(BrowserThread::UI));
  return "chrome-ext-ansg";
}

// It's acutally OK to call this method on any thread, but it's currently placed
// in UIThreadSearchTermsData since SearchTermsData cannot depend on src/chrome
// as it is shared with iOS.
std::string UIThreadSearchTermsData::GoogleImageSearchSource() const {
  return std::string();
}

// static
void UIThreadSearchTermsData::SetGoogleBaseURL(const std::string& base_url) {
  delete google_base_url_;
  google_base_url_ = base_url.empty() ? NULL : new std::string(base_url);
}
