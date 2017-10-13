// Copyright 2016 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_BRAVE_JAVASCRIPT_DIALOG_MANAGER_H_
#define BRAVE_BROWSER_BRAVE_JAVASCRIPT_DIALOG_MANAGER_H_

#include <map>
#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/time/time.h"
#include "content/public/browser/javascript_dialog_manager.h"

namespace brave {

class JavaScriptDialogExtraData {
 public:
  JavaScriptDialogExtraData();

  // The number of dialogs that the webContents has opened
  int dialog_count_;

  // True if the user has decided to block future JavaScript dialogs.
  bool suppress_javascript_messages_;

  // Number of dialogs from the origin that were suppressed.
  int suppressed_dialog_count_;
};

class BraveJavaScriptDialogManager : public content::JavaScriptDialogManager {
 public:
  typedef std::map<void*, JavaScriptDialogExtraData> ExtraDataMap;
  using DialogClosedCallback =
      content::JavaScriptDialogManager::DialogClosedCallback;
  static BraveJavaScriptDialogManager* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveJavaScriptDialogManager>;

  BraveJavaScriptDialogManager();
  ~BraveJavaScriptDialogManager() override;

  // JavaScriptDialogManager:
  void RunJavaScriptDialog(content::WebContents* web_contents,
                           const GURL& alerting_frame_url,
                           content::JavaScriptDialogType dialog_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override;
  void RunBeforeUnloadDialog(content::WebContents* web_contents,
                             bool is_reload,
                             DialogClosedCallback callback) override;
  bool HandleJavaScriptDialog(content::WebContents* web_contents,
                              bool accept,
                              const base::string16* prompt_override) override;
  void CancelDialogs(content::WebContents* web_contents,
                     bool reset_state) override;

  std::string GetEventName(content::JavaScriptDialogType dialog_type);
  base::string16 GetTitle(content::WebContents* web_contents,
                          const GURL& origin_url,
                          bool is_alert);

  // Wrapper around OnDialogClosed; logs UMA stats before continuing on.
  void OnBeforeUnloadDialogClosed(content::WebContents* web_contents,
                                  DialogClosedCallback callback,
                                  bool success,
                                  const base::string16& user_input,
                                  bool suppress_javascript_messages);

  // Wrapper around a DialogClosedCallback so that we can intercept it before
  // passing it onto the original callback.
  void OnDialogClosed(content::WebContents* web_contents,
                      DialogClosedCallback callback,
                      bool success,
                      const base::string16& user_input,
                      bool suppress_javascript_messages);

  // Mapping between the WebContents and their extra data. The key
  // is a void* because the pointer is just a cookie and is never dereferenced.
  ExtraDataMap javascript_dialog_extra_data_;

  // Record a single create and close timestamp to track the time between
  // dialogs.
  base::TimeTicks last_close_time_;
  base::TimeTicks last_creation_time_;

  DISALLOW_COPY_AND_ASSIGN(BraveJavaScriptDialogManager);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_JAVASCRIPT_DIALOG_MANAGER_H_
