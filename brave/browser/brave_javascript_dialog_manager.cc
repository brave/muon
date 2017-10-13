// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/brave_javascript_dialog_manager.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/bind.h"
#include "base/i18n/rtl.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "components/url_formatter/elide_url.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/javascript_dialog_type.h"
#include "gin/converter.h"
#include "gin/try_catch.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/font_list.h"

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/node_includes.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"

namespace mate {

template<>
struct Converter<brave::JavaScriptDialogExtraData*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   brave::JavaScriptDialogExtraData* val) {
    if (!val)
      return v8::Null(isolate);

    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("suppressJavascriptMessages", val->suppress_javascript_messages_);
    dict.Set("dialogCount", val->dialog_count_);
    dict.Set("suppressedDialogCount", val->suppressed_dialog_count_);
    return dict.GetHandle();
  }
};

}  // namespace mate

namespace brave {

namespace {

// Keep in sync with kDefaultMessageWidth, but allow some space for the rest of
// the text.
const int kUrlElideWidth = 350;

bool ShouldDisplaySuppressCheckbox(
    JavaScriptDialogExtraData* extra_data) {
  return extra_data->dialog_count_ > 0;
}

void LogUMAMessageLengthStats(const base::string16& message) {
  UMA_HISTOGRAM_COUNTS("JSDialogs.CountOfJSDialogMessageCharacters",
                       static_cast<int32_t>(message.length()));

  int32_t newline_count =
      std::count_if(message.begin(), message.end(),
                    [](const base::char16& c) { return c == '\n'; });
  UMA_HISTOGRAM_COUNTS("JSDialogs.CountOfJSDialogMessageNewlines",
                       newline_count);
}

}  // namespace

JavaScriptDialogExtraData::JavaScriptDialogExtraData()
    : dialog_count_(0),
      suppress_javascript_messages_(false),
      suppressed_dialog_count_(0) {}

////////////////////////////////////////////////////////////////////////////////
// BraveJavaScriptDialogManager, public:

// static
BraveJavaScriptDialogManager* BraveJavaScriptDialogManager::GetInstance() {
  return base::Singleton<BraveJavaScriptDialogManager>::get();
}

////////////////////////////////////////////////////////////////////////////////
// BraveJavaScriptDialogManager, private:

BraveJavaScriptDialogManager::BraveJavaScriptDialogManager() {
}

BraveJavaScriptDialogManager::~BraveJavaScriptDialogManager() {
}

void BraveJavaScriptDialogManager::RunJavaScriptDialog(
    content::WebContents* web_contents,
    const GURL& alerting_frame_url,
    content::JavaScriptDialogType dialog_type,
    const base::string16& message_text,
    const base::string16& default_prompt_text,
    DialogClosedCallback callback,
    bool* did_suppress_message) {
  *did_suppress_message = false;

  JavaScriptDialogExtraData* extra_data =
      &javascript_dialog_extra_data_[web_contents];

  if (extra_data->suppress_javascript_messages_) {
    // If a page tries to open dialogs in a tight loop, the number of
    // suppressions logged can grow out of control. Arbitrarily cap the number
    // logged at 100. That many suppressed dialogs is enough to indicate the
    // page is doing something very hinky.
    if (extra_data->suppressed_dialog_count_ < 100) {
      // Log a suppressed dialog as one that opens and then closes immediately.
      UMA_HISTOGRAM_MEDIUM_TIMES(
          "JSDialogs.FineTiming.TimeBetweenDialogCreatedAndSameDialogClosed",
          base::TimeDelta());

      // Only increment the count if it's not already at the limit, to prevent
      // overflow.
      extra_data->suppressed_dialog_count_++;
    }

    *did_suppress_message = true;
    return;
  }

  base::TimeTicks now = base::TimeTicks::Now();
  if (!last_creation_time_.is_null()) {
    // A new dialog has been created: log the time since the last one was
    // created.
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "JSDialogs.FineTiming.TimeBetweenDialogCreatedAndNextDialogCreated",
        now - last_creation_time_);
  }
  last_creation_time_ = now;

  // Also log the time since a dialog was closed, but only if this is the first
  // dialog that was opened since the closing.
  if (!last_close_time_.is_null()) {
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "JSDialogs.FineTiming.TimeBetweenDialogClosedAndNextDialogCreated",
        now - last_close_time_);
    last_close_time_ = base::TimeTicks();
  }

  bool is_alert = dialog_type == content::JAVASCRIPT_DIALOG_TYPE_ALERT;
  base::string16 dialog_title =
      GetTitle(web_contents, alerting_frame_url, is_alert);

  LogUMAMessageLengthStats(message_text);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate) {
    *did_suppress_message = true;
    return;
  }

  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env) {
    *did_suppress_message = true;
    return;
  }

  gin::TryCatch try_catch(isolate);
  mate::EmitEvent(isolate, env->process_object(), GetEventName(dialog_type),
                  web_contents, extra_data, dialog_title, message_text,
                  default_prompt_text,
                  ShouldDisplaySuppressCheckbox(extra_data),
                  false,  // is_before_unload_dialog
                  false,  // is_reload
                  base::BindOnce(&BraveJavaScriptDialogManager::OnDialogClosed,
                                 base::Unretained(this), web_contents,
                                 std::move(callback)));
  if (try_catch.HasCaught()) {
    std::move(callback).Run(false, base::string16());
    LOG(ERROR) << "Uncaught exception: " << try_catch.GetStackTrace();
  } else {
    if (extra_data->dialog_count_ < 100) {
      extra_data->dialog_count_++;
    }
  }
}

void BraveJavaScriptDialogManager::RunBeforeUnloadDialog(
    content::WebContents* web_contents,
    bool is_reload,
    DialogClosedCallback callback) {
  JavaScriptDialogExtraData* extra_data =
      &javascript_dialog_extra_data_[web_contents];

  if (extra_data->suppress_javascript_messages_) {
    // If a site harassed the user enough for them to put it on mute, then it
    // lost its privilege to deny unloading.
    std::move(callback).Run(true, base::string16());
    return;
  }

  // Build the dialog message. We explicitly do _not_ allow the webpage to
  // specify the contents of this dialog, because most of the time nowadays it's
  // used for scams.
  //
  // This does not violate the spec. Per
  // https://html.spec.whatwg.org/#prompt-to-unload-a-document, step 7:
  //
  // "The prompt shown by the user agent may include the string of the
  // returnValue attribute, or some leading subset thereof."
  //
  // The prompt MAY include the string. It doesn't any more. Scam web page
  // authors have abused this, so we're taking away the toys from everyone. This
  // is why we can't have nice things.
  const base::string16 title = l10n_util::GetStringUTF16(is_reload ?
      IDS_BEFORERELOAD_MESSAGEBOX_TITLE : IDS_BEFOREUNLOAD_MESSAGEBOX_TITLE);
  const base::string16 message =
      l10n_util::GetStringUTF16(IDS_BEFOREUNLOAD_MESSAGEBOX_MESSAGE);

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate) {
    std::move(callback).Run(true, base::string16());
    return;
  }

  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env) {
    std::move(callback).Run(true, base::string16());
    return;
  }

  gin::TryCatch try_catch(isolate);
  mate::EmitEvent(
      isolate, env->process_object(),
      GetEventName(content::JAVASCRIPT_DIALOG_TYPE_CONFIRM), web_contents,
      extra_data, title, message,
      base::string16(),  // default_prompt_text
      ShouldDisplaySuppressCheckbox(extra_data),
      true,  // is_before_unload_dialog
      is_reload,
      base::BindOnce(&BraveJavaScriptDialogManager::OnBeforeUnloadDialogClosed,
                     base::Unretained(this), web_contents,
                     std::move(callback)));

  if (try_catch.HasCaught()) {
    std::move(callback).Run(true, base::string16());
    LOG(ERROR) << "Uncaught exception: " << try_catch.GetStackTrace();
  } else {
    if (extra_data->dialog_count_ < 100) {
        extra_data->dialog_count_++;
    }
  }
}

bool BraveJavaScriptDialogManager::HandleJavaScriptDialog(
    content::WebContents* web_contents,
    bool accept,
    const base::string16* prompt_override) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return false;

  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env)
    return false;

  gin::TryCatch try_catch(isolate);
  mate::EmitEvent(isolate,
      env->process_object(),
      "handle-javascript-dialog",
      web_contents,
      accept,
      *prompt_override);

  if (try_catch.HasCaught()) {
    LOG(ERROR) << "Uncaught exception: " << try_catch.GetStackTrace();
    return false;
  } else {
    return true;
  }
}

std::string
BraveJavaScriptDialogManager::GetEventName(
    content::JavaScriptDialogType dialog_type) {
  switch (dialog_type) {
    case content::JAVASCRIPT_DIALOG_TYPE_ALERT: return "window-alert";
    case content::JAVASCRIPT_DIALOG_TYPE_CONFIRM: return "window-confirm";
    case content::JAVASCRIPT_DIALOG_TYPE_PROMPT: return "window-prompt";
    default: return "unknown";
  }
}

base::string16 BraveJavaScriptDialogManager::GetTitle(
    content::WebContents* web_contents,
    const GURL& origin_url,
    bool is_alert) {
  // For extensions, show the extension name, but only if the origin of
  // the alert matches the top-level WebContents.
  std::string name;
  // if (extensions_client_->GetExtensionName(web_contents, origin_url, &name))
  //   return base::UTF8ToUTF16(name);

  // Otherwise, return the formatted URL. For non-standard URLs such as |data:|,
  // just say "This page".
  bool is_same_origin_as_main_frame =
      (web_contents->GetURL().GetOrigin() == origin_url.GetOrigin());
  if (origin_url.IsStandard() && !origin_url.SchemeIsFile() &&
      !origin_url.SchemeIsFileSystem()) {
    base::string16 url_string =
        url_formatter::ElideHost(origin_url, gfx::FontList(), kUrlElideWidth);
    return l10n_util::GetStringFUTF16(
        is_same_origin_as_main_frame ? IDS_JAVASCRIPT_MESSAGEBOX_TITLE
                                     : IDS_JAVASCRIPT_MESSAGEBOX_TITLE_IFRAME,
        base::i18n::GetDisplayStringInLTRDirectionality(url_string));
  }
  return l10n_util::GetStringUTF16(
      is_same_origin_as_main_frame
          ? IDS_JAVASCRIPT_MESSAGEBOX_TITLE_NONSTANDARD_URL
          : IDS_JAVASCRIPT_MESSAGEBOX_TITLE_NONSTANDARD_URL_IFRAME);
}

void BraveJavaScriptDialogManager::CancelDialogs(
    content::WebContents* web_contents,
    bool reset_state) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  if (!isolate)
    return;

  node::Environment* env = node::Environment::GetCurrent(isolate);
  if (!env)
    return;

  gin::TryCatch try_catch(isolate);
  mate::EmitEvent(isolate,
      env->process_object(),
      "cancel-all-javascript-dialogs",
      web_contents);
  if (try_catch.HasCaught()) {
    LOG(ERROR) << "Uncaught exception: " << try_catch.GetStackTrace();
  }
}

void BraveJavaScriptDialogManager::OnBeforeUnloadDialogClosed(
    content::WebContents* web_contents,
    DialogClosedCallback callback,
    bool success,
    const base::string16& user_input,
    bool suppress_javascript_messages = false) {
  enum class StayVsLeave {
    STAY = 0,
    LEAVE = 1,
    MAX,
  };
  UMA_HISTOGRAM_ENUMERATION(
      "JSDialogs.OnBeforeUnloadStayVsLeave",
      static_cast<int>(success ? StayVsLeave::LEAVE : StayVsLeave::STAY),
      static_cast<int>(StayVsLeave::MAX));

  OnDialogClosed(web_contents, std::move(callback), success, user_input,
                 suppress_javascript_messages);
}

void BraveJavaScriptDialogManager::OnDialogClosed(
    content::WebContents* web_contents,
    DialogClosedCallback callback,
    bool success,
    const base::string16& user_input,
    bool suppress_javascript_messages = false) {
  JavaScriptDialogExtraData* extra_data =
      &javascript_dialog_extra_data_[web_contents];
  extra_data->suppress_javascript_messages_ = suppress_javascript_messages;
  // If an extension opened this dialog then the extension may shut down its
  // lazy background page after the dialog closes. (Dialogs are closed before
  // their WebContents is destroyed so |web_contents| is still valid here.)
  // extensions_client_->OnDialogClosed(web_contents);

  last_close_time_ = base::TimeTicks::Now();

  std::move(callback).Run(success, user_input);
}

}  // namespace brave
