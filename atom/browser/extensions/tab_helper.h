// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_
#define ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_

#include <memory>
#include <string>

#include "atom/browser/native_window_observer.h"
#include "base/macros.h"
#include "chrome/browser/ui/browser_list_observer.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/script_execution_observer.h"
#include "extensions/browser/script_executor.h"

class Browser;

namespace base {
class DictionaryValue;
}

namespace brave {
class TabViewGuest;
}

namespace content {
class BrowserContext;
class RenderFrameHost;
class RenderViewHost;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace keys {
extern const char kIdKey[];
extern const char kIncognitoKey[];
extern const char kWindowIdKey[];
extern const char kAudibleKey[];
extern const char kMutedKey[];
}

using guest_view::GuestViewManager;

namespace extensions {

class Extension;

// This class keeps the extension API's windowID up-to-date with the current
// window of the tab.
class TabHelper : public content::WebContentsObserver,
                  public content::WebContentsUserData<TabHelper>,
                  public BrowserListObserver,
                  public atom::NativeWindowObserver,
                  public TabStripModelObserver {
 public:
  ~TabHelper() override;

  static void CreateTab(content::WebContents* owner,
                content::BrowserContext* browser_context,
                const base::DictionaryValue& create_params,
                const GuestViewManager::WebContentsCreatedCallback& callback);
  static content::WebContents* CreateTab(content::WebContents* owner,
                            content::WebContents::CreateParams create_params);
  static void DestroyTab(content::WebContents* tab);

  bool AttachGuest(int window_id, int index);
  content::WebContents* DetachGuest();

  // Identifier of the tab.
  void SetTabId(content::RenderFrameHost* render_frame_host);
  int32_t session_id() const;

  // Identifier of the window the tab is in.
  void SetWindowId(const int32_t& id);
  int32_t window_id() const;

  void SetBrowser(Browser* browser);
  // Set the tab's index in its window
  void SetTabIndex(int index);

  void SetAutoDiscardable(bool auto_discardable);

  void SetActive(bool active);

  void SetPinned(bool pinned);
  bool IsPinned() const;

  bool Discard();

  bool IsDiscarded();

  void DidAttach();

  void SetTabValues(const base::DictionaryValue& values);
  base::DictionaryValue* getTabValues() {
    return values_.get();
  }

  void SetOpener(int openerTabId);

  bool ExecuteScriptInTab(mate::Arguments* args);

  ScriptExecutor* script_executor() const {
    return script_executor_.get();
  }

  Browser* browser() const {
    return browser_;
  }

  brave::TabViewGuest* guest() const;

  int get_index() const;
  bool is_pinned() const { return pinned_; }
  bool is_active() const;

  void SetPlaceholder(bool is_placeholder);
  bool is_placeholder() const { return is_placeholder_; }

  int opener_tab_id() const { return opener_tab_id_; }

  // If the specified WebContents has a TabHelper (probably because it
  // was used as the contents of a tab), returns a tab id. This value is
  // immutable for a given tab. It will be unique across Chrome within the
  // current session, but may be re-used across sessions. Returns -1
  // for a NULL WebContents or if the WebContents has no TabHelper.
  static int32_t IdForTab(const content::WebContents* tab);

  static content::WebContents* GetTabById(int tab_id,
                         content::BrowserContext* browser_context);
  static content::WebContents* GetTabById(int32_t tab_id);

  static int GetTabStripIndex(int window_id, int index);

  static int32_t IdForWindowContainingTab(
      const content::WebContents* tab);

 private:
  explicit TabHelper(content::WebContents* contents);
  friend class content::WebContentsUserData<TabHelper>;

  void TabDetachedAt(content::WebContents* contents, int index) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;

  void OnBrowserRemoved(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;
  void UpdateBrowser(Browser* browser);

  void MaybeAttachOrCreatePinnedTab();
  void MaybeRequestWindowClose();

  // atom::NativeWindowObserver overrides.
  void WillCloseWindow(bool* prevent_default) override;

  void ExecuteScript(
      std::string extension_id,
      std::unique_ptr<base::DictionaryValue> options,
      extensions::ScriptExecutor::ResultType result,
      extensions::ScriptExecutor::ExecuteScriptCallback callback,
      const GURL& file_url,
      bool success,
      std::unique_ptr<std::string> code_string);

  // content::WebContentsObserver overrides.
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;
  void RenderFrameCreated(content::RenderFrameHost* host) override;
  void WebContentsDestroyed() override;
  void DidCloneToNewWebContents(
      content::WebContents* old_web_contents,
      content::WebContents* new_web_contents) override;
  void WasShown() override;

  // Our content script observers. Declare at top so that it will outlive all
  // other members, since they might add themselves as observers.
  base::ObserverList<ScriptExecutionObserver> script_execution_observers_;

  std::unique_ptr<base::DictionaryValue> values_;
  std::unique_ptr<ScriptExecutor> script_executor_;

  // Index of the tab within the window
  int index_;
  bool pinned_;
  bool discarded_;
  bool active_;
  bool is_placeholder_;
  bool window_closing_;
  int opener_tab_id_;

  Browser* browser_;

  DISALLOW_COPY_AND_ASSIGN(TabHelper);
};

}  // namespace extensions

#endif  // ATOM_BROWSER_EXTENSIONS_TAB_HELPER_H_
