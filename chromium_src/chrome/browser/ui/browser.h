// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_BROWSER_H_
#define CHROME_BROWSER_UI_BROWSER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/strings/string16.h"
#include "build/build_config.h"
#include "components/sessions/core/session_id.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "ui/base/ui_base_types.h"
#include "ui/base/window_open_disposition.h"
#include "ui/gfx/geometry/rect.h"

class BrowserWindow;
class Profile;
class ScopedKeepAlive;
class TabStripModel;
class TabStripModelDelegate;

class Browser : public content::WebContentsDelegate {
 public:
  // SessionService::WindowType mirrors these values.  If you add to this
  // enum, look at SessionService::WindowType to see if it needs to be
  // updated.
  enum Type {
    // If you add a new type, consider updating the test
    // BrowserTest.StartMaximized.
    TYPE_TABBED = 1,
    TYPE_POPUP = 2
  };

  // Possible elements of the Browser window.
  enum WindowFeature {
    FEATURE_NONE = 0,
    FEATURE_TITLEBAR = 1,
    FEATURE_TABSTRIP = 2,
    FEATURE_TOOLBAR = 4,
    FEATURE_LOCATIONBAR = 8,
    FEATURE_BOOKMARKBAR = 16,
    FEATURE_INFOBAR = 32,
    FEATURE_DOWNLOADSHELF = 64,
    FEATURE_WEBAPPFRAME = 128
  };

  struct CreateParams {
    explicit CreateParams(Profile* profile);
    CreateParams(Type type, Profile* profile);
    CreateParams(Type type, Profile* profile, bool user_gesture) {}
    CreateParams(const CreateParams& other);

    // The browser type.
    Type type;

    // The associated profile.
    Profile* profile;

    // Specifies the browser is_trusted_source_ value.
    bool trusted_source;

    // The bounds of the window to open.
    gfx::Rect initial_bounds;

    // The workspace the window should open in, if the platform supports it.
    std::string initial_workspace;

    ui::WindowShowState initial_show_state;

    bool is_session_restore;

    // Supply a custom BrowserWindow implementation, to be used instead of the
    // default. Intended for testing.
    BrowserWindow* window;

   private:
    friend class Browser;

    // The application name that is also the name of the window to the shell.
    // Do not set this value directly, use CreateForApp.
    // This name will be set for:
    // 1) v1 applications launched via an application shortcut or extension API.
    // 2) undocked devtool windows.
    // 3) popup windows spawned from v1 applications.
    std::string app_name;
  };

  // Constructors, Creation, Showing //////////////////////////////////////////

  explicit Browser(const CreateParams& params);
  ~Browser();

  // void WillDestroyNativeObject();

  // ui::WindowShowState initial_show_state() const { return initial_show_state_; }
  // void set_initial_show_state(ui::WindowShowState initial_show_state) {
  //   initial_show_state_ = initial_show_state;
  // }

  // // Set indicator that this browser is being created via session restore.
  // // This is used on the Mac (only) to determine animation style when the
  // // browser window is shown.
  // void set_is_session_restore(bool is_session_restore) {
  //   is_session_restore_ = is_session_restore;
  // }
  // bool is_session_restore() const {
  //   return is_session_restore_;
  // }

  // Accessors ////////////////////////////////////////////////////////////////

  // Type type() const { return type_; }
  // const std::string& app_name() const { return app_name_; }
  // bool is_trusted_source() const { return is_trusted_source_; }
  Profile* profile() const { return profile_; }

  BrowserWindow* window() const { return window_; }
  TabStripModel* tab_strip_model() const { return tab_strip_model_.get(); }
  SessionID& session_id() { return session_id_; }

  // OnBeforeUnload handling //////////////////////////////////////////////////

  // Gives beforeunload handlers the chance to cancel the close. Returns whether
  // to proceed with the close. If called while the process begun by
  // TryToCloseWindow is in progress, returns false without taking
  // action.
  // bool ShouldCloseWindow();

  // Begins the process of confirming whether the associated browser can be
  // closed. If there are no tabs with beforeunload handlers it will immediately
  // return false. If |skip_beforeunload| is true, all beforeunload
  // handlers will be skipped and window closing will be confirmed without
  // showing the prompt, the function will return false as well.
  // Otherwise, it starts prompting the user, returns true and will call
  // |on_close_confirmed| with the result of the user's decision.
  // After calling this function, if the window will not be closed, call
  // ResetBeforeUnloadHandlers() to reset all beforeunload handlers; calling
  // this function multiple times without an intervening call to
  // ResetTryToCloseWindow() will run only the beforeunload handlers
  // registered since the previous call.
  // Note that if the browser window has been used before, users should always
  // have a chance to save their work before the window is closed without
  // triggering beforeunload event.
  bool TryToCloseWindow(bool skip_beforeunload,
      const base::Callback<void(bool)>& on_close_confirmed);

  bool TabsNeedBeforeUnloadFired();
  // Clears the results of any beforeunload confirmation dialogs triggered by a
  // TryToCloseWindow call.
  void ResetTryToCloseWindow();

  // Assorted browser commands ////////////////////////////////////////////////

  // NOTE: Within each of the following sections, the IDs are ordered roughly by
  // how they appear in the GUI/menus (left to right, top to bottom, etc.).

  // See the description of
  // FullscreenController::ToggleFullscreenModeWithExtension.
  // void ToggleFullscreenModeWithExtension(const GURL& extension_url);

  // Returns true if the Browser supports the specified feature. The value of
  // this varies during the lifetime of the browser. For example, if the window
  // is fullscreen this may return a different value. If you only care about
  // whether or not it's possible for the browser to support a particular
  // feature use |CanSupportWindowFeature|.
  bool SupportsWindowFeature(WindowFeature feature) const;

  // Returns true if the Browser can support the specified feature. See comment
  // in |SupportsWindowFeature| for details on this.
  bool CanSupportWindowFeature(WindowFeature feature) const;

  // Used to register a KeepAlive to affect the Chrome lifetime. The KeepAlive
  // is registered when the browser is added to the browser list, and unregisted
  // when it is removed from it.
  void RegisterKeepAlive();
  void UnregisterKeepAlive();

  bool is_type_tabbed() const { return type_ == TYPE_TABBED; }
  // bool is_type_popup() const { return type_ == TYPE_POPUP; }

  bool is_app() const;
  bool is_devtools() const { return false; }

  bool ShouldRunUnloadListenerBeforeClosing(content::WebContents* web_contents);
  bool RunUnloadListenerBeforeClosing(content::WebContents* web_contents);

 private:

  // Implementation of SupportsWindowFeature and CanSupportWindowFeature. If
  // |check_fullscreen| is true, the set of features reflect the actual state of
  // the browser, otherwise the set of features reflect the possible state of
  // the browser.
  bool SupportsWindowFeatureImpl(WindowFeature feature,
                                 bool check_fullscreen) const;

  // This Browser's type.
  const Type type_;

  // This Browser's profile.
  Profile* const profile_;

  // This Browser's window.
  BrowserWindow* window_;

  std::unique_ptr<TabStripModelDelegate> tab_strip_model_delegate_;
  std::unique_ptr<TabStripModel> tab_strip_model_;

  // The application name that is also the name of the window to the shell.
  // This name should be set when:
  // 1) we launch an application via an application shortcut or extension API.
  // 2) we launch an undocked devtool window.
  std::string app_name_;

  // True if the source is trusted (i.e. we do not need to show the URL in a
  // a popup window). Also used to determine which app windows to save and
  // restore on Chrome OS.
  bool is_trusted_source_;

  // Unique identifier of this browser for session restore. This id is only
  // unique within the current session, and is not guaranteed to be unique
  // across sessions.
  SessionID session_id_;

  // /////////////////////////////////////////////////////////////////////////////

  // // Override values for the bounds of the window and its maximized or minimized
  // // state.
  // // These are supplied by callers that don't want to use the default values.
  // // The default values are typically loaded from local state (last session),
  // // obtained from the last window of the same type, or obtained from the
  // // shell shortcut's startup info.
  // gfx::Rect override_bounds_;
  ui::WindowShowState initial_show_state_;
  // const std::string initial_workspace_;

  // Tracks when this browser is being created by session restore.
  bool is_session_restore_;

  std::unique_ptr<ScopedKeepAlive> keep_alive_;

  // The following factory is used to close the frame at a later time.
  base::WeakPtrFactory<Browser> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Browser);
};

#endif  // CHROME_BROWSER_UI_BROWSER_H_
