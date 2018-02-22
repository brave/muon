// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_

#include <memory>
#include <string>
#include <vector>

#include "atom/browser/api/save_page_handler.h"
#include "atom/browser/api/trackable_object.h"
#include "atom/browser/common_web_contents_delegate.h"
#include "atom/common/options_switches.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "content/common/cursors/webcursor.h"
#include "content/common/view_messages.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/context_menu_params.h"
#include "content/public/common/favicon_url.h"
#include "extensions/features/features.h"
#include "ipc/ipc_sender.h"
#include "native_mate/handle.h"
#include "ui/gfx/image/image.h"

class ProtocolHandler;
class TabStripModel;

namespace autofill {
class AtomAutofillClient;
}

namespace base {
class SharedMemory;
class SharedMemoryHandle;
}

namespace brave {
class TabViewGuest;
}

namespace guest_view {
class GuestViewBase;
}

namespace brightray {
class InspectableWebContents;
}

namespace gfx {
class Size;
}

namespace mate {

template<>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case WindowOpenDisposition::CURRENT_TAB:
        disposition = "default"; break;
      case WindowOpenDisposition::NEW_FOREGROUND_TAB:
        disposition = "foreground-tab"; break;
      case WindowOpenDisposition::NEW_BACKGROUND_TAB:
        disposition = "background-tab"; break;
      case WindowOpenDisposition::NEW_POPUP:
        disposition = "new-popup"; break;
      case WindowOpenDisposition::NEW_WINDOW:
        disposition = "new-window"; break;
      default: disposition = "other"; break;
    }
    return mate::ConvertToV8(isolate, disposition);
  }
};

class Arguments;
class Dictionary;
}  // namespace mate

namespace atom {

struct SetSizeParams {
  SetSizeParams() {}
  ~SetSizeParams() {}

  std::unique_ptr<bool> enable_auto_size;
  std::unique_ptr<gfx::Size> min_size;
  std::unique_ptr<gfx::Size> max_size;
  std::unique_ptr<gfx::Size> normal_size;
};

class AtomBrowserContext;

namespace api {

class WebContents : public mate::TrackableObject<WebContents>,
                    public CommonWebContentsDelegate,
                    public content::WebContentsObserver,
                    public TabStripModelObserver {
 public:
  enum Type {
    BACKGROUND_PAGE,  // A DevTools extension background page.
    BROWSER_WINDOW,  // Used by BrowserWindow.
    REMOTE,  // Thin wrap around an existing WebContents.
    WEB_VIEW,  // Used by <webview>.
  };

  // For node.js callback function type: function(error, buffer)
  using PrintToPDFCallback =
      base::Callback<void(v8::Local<v8::Value>, v8::Local<v8::Value>)>;

  // Get the webcontents by tabId.
  static mate::Handle<WebContents> FromTabID(
    v8::Isolate* isolate, int tab_id);

  static void CreateTab(mate::Arguments* args);

  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, content::WebContents* web_contents);

  static mate::Handle<WebContents> CreateFrom(
      v8::Isolate* isolate, content::WebContents* web_contents, Type type);

  // Create a new WebContents.
  static mate::Handle<WebContents> Create(
      v8::Isolate* isolate, const mate::Dictionary& options);

  static mate::Handle<WebContents> CreateWithParams(
      v8::Isolate* isolate,
      const mate::Dictionary& options,
      const content::WebContents::CreateParams& create_params);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  brightray::InspectableWebContents* managed_web_contents() const override;

  void Clone(mate::Arguments* args);

  void DestroyWebContents();

  void SetOwnerWindow(NativeWindow* owner_window) override;
  void SetOwnerWindow(content::WebContents* web_contents,
                      NativeWindow* owner_window) override;

  int GetID() const;
  Type GetType() const;
  int GetGuestInstanceId() const;
  mate::Handle<WebContents> GetMainFrame() const;
  bool Equal(const WebContents* web_contents) const;
  void LoadURL(const GURL& url, const mate::Dictionary& options);
  void Reload(bool ignore_cache);
  void DownloadURL(const GURL& url, mate::Arguments* args);

  GURL GetURL() const;
  base::string16 GetTitle() const;
  bool IsInitialBlankNavigation() const;
  bool IsLoading() const;
  bool IsLoadingMainFrame() const;
  bool IsWaitingForResponse() const;
  void Stop();
  void ReloadIgnoringCache();
  void GoBack();
  void GoForward();
  void GoToOffset(int offset);
  bool CanGoToOffset(int offset) const;
  bool CanGoBack() const;
  bool CanGoForward() const;
  void GoToIndex(int index);
  const GURL& GetURLAtIndex(int index) const;
  const base::string16 GetTitleAtIndex(int index) const;
  int GetCurrentEntryIndex() const;
  int GetLastCommittedEntryIndex() const;
  int GetEntryCount() const;
  void GetController(mate::Arguments* args) const;
  const std::string& GetWebRTCIPHandlingPolicy() const;
  void SetWebRTCIPHandlingPolicy(const std::string webrtc_ip_handling_policy);
  void ShowRepostFormWarningDialog(content::WebContents* source) override;
  bool IsCrashed() const;
  void SetUserAgent(const std::string& user_agent, mate::Arguments* args);
  std::string GetUserAgent();
  bool SavePage(const base::FilePath& full_file_path,
                const content::SavePageType& save_type,
                const SavePageHandler::SavePageCallback& callback);
  void OpenDevTools(mate::Arguments* args);
  void CloseDevTools();
  bool IsDevToolsOpened();
  bool IsDevToolsFocused();
  void ToggleDevTools();
  void InspectElement(int x, int y);
  void InspectServiceWorker();
  void HasServiceWorker(
      const base::Callback<void(content::ServiceWorkerCapability capability)>&);
  void UnregisterServiceWorker(const base::Callback<void(bool)>&);
  void SetAudioMuted(bool muted);
  bool IsAudioMuted();
  void Print(mate::Arguments* args);
  int GetContentWindowId();
  void ResumeLoadingCreatedWebContents();

  // Print current page as PDF.
  void PrintToPDF(const base::DictionaryValue& setting,
                  const PrintToPDFCallback& callback);

  // DevTools workspace api.
  void AddWorkSpace(mate::Arguments* args, const base::FilePath& path);
  void RemoveWorkSpace(mate::Arguments* args, const base::FilePath& path);

  // Editing commands.
  void Undo();
  void Redo();
  void Cut();
  void Copy();
  void Paste();
  void PasteAndMatchStyle();
  void Delete();
  void SelectAll();
  void CollapseSelection();
  void Replace(const base::string16& word);
  void ReplaceMisspelling(const base::string16& word);
  uint32_t FindInPage(mate::Arguments* args);
  void StopFindInPage(content::StopFindAction action);
  void ShowCertificate();
  void ShowDefinitionForSelection();
  void CopyImageAt(int x, int y);

  // Focus.
  void Focus();
  bool IsFocused() const;

  // Tab state
  void SetActive(bool active);
  void SetTabIndex(int index);
  void SetPinned(bool pinned);
  void SetAutoDiscardable(bool auto_discardable);
  void Discard();

  // Zoom
  void SetZoomLevel(double zoom);
  void ZoomIn();
  void ZoomOut();
  void ZoomReset();
  int GetZoomPercent();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  bool ExecuteScriptInTab(mate::Arguments* args);
  void SetTabValues(const base::DictionaryValue& values);
#endif

  // Send messages to browser.
  static bool SendIPCMessage(int render_process_id,
                             int render_frame_id,
                             const base::string16& channel,
                             const base::ListValue& args);
  static bool SendIPCSharedMemory(int render_process_id,
                                  int render_frame_id,
                                  const base::string16& channel,
                                  base::SharedMemory* shared_memory);

  // Send WebInputEvent to the page.
  void SendInputEvent(v8::Isolate* isolate, v8::Local<v8::Value> input_event);

  // Dragging native items.
  void StartDrag(const mate::Dictionary& item, mate::Arguments* args);

  // Captures the page with |rect|, |callback| would be called when capturing is
  // done.
  void CapturePage(mate::Arguments* args);

  void EnablePreferredSizeMode(bool enable);
  void GetPreferredSize(mate::Arguments* args);

  // Methods for creating <webview>.
  void SetSize(const SetSizeParams& params);
  bool IsGuest() const;
  bool IsRemote() const;

  // Callback triggered on permission response.
  void OnEnterFullscreenModeForTab(content::WebContents* source,
                                   const GURL& origin,
                                   bool allowed);

  void AutofillSelect(const std::string& value, int frontend_id, int index);
  void AutofillPopupHidden();

  void AttachGuest(mate::Arguments* args);
  void DetachGuest(mate::Arguments* args);
  void IsPlaceholder(mate::Arguments* args);

  void SavePassword();
  void NeverSavePassword();
  void UpdatePassword();
  void NoUpdatePassword();

  // Returns the web preferences of current WebContents.
  v8::Local<v8::Value> GetWebPreferences(v8::Isolate* isolate);

  // Returns the owner window.
  v8::Local<v8::Value> GetOwnerBrowserWindow();

  // Properties.
  int32_t ID() const;
  v8::Local<v8::Value> Session(v8::Isolate* isolate);
  content::WebContents* HostWebContents();
  v8::Local<v8::Value> DevToolsWebContents(v8::Isolate* isolate);
  v8::Local<v8::Value> Debugger(v8::Isolate* isolate);

 protected:
  WebContents(v8::Isolate* isolate,
      content::WebContents* web_contents, Type type);
  WebContents(v8::Isolate* isolate, const mate::Dictionary& options);
  WebContents(v8::Isolate* isolate, const mate::Dictionary& options,
      const content::WebContents::CreateParams& create_params);
  ~WebContents();

  void CreateWebContents(v8::Isolate* isolate,
      const mate::Dictionary& options,
      const content::WebContents::CreateParams& create_params);
  void CompleteInit(v8::Isolate* isolate,
      content::WebContents *web_contents,
      const mate::Dictionary& options);

  void OnTabCreated(const mate::Dictionary& options,
      base::Callback<void(content::WebContents*)> callback,
      content::WebContents* tab);

  void OnCloneCreated(const mate::Dictionary& options,
      base::Callback<void(content::WebContents*)> callback,
      content::WebContents* clone);

  void AuthorizePlugin(mate::Arguments* args);

  // TabStripModelObserver
  void TabPinnedStateChanged(TabStripModel* tab_strip_model,
                             content::WebContents* contents,
                             int index) override;
  void TabDetachedAt(content::WebContents* contents, int index) override;
  void ActiveTabChanged(content::WebContents* old_contents,
                        content::WebContents* new_contents,
                        int index,
                        int reason) override;
  void TabInsertedAt(TabStripModel* tab_strip_model,
                     content::WebContents* contents,
                     int index,
                     bool foreground) override;
  void TabMoved(content::WebContents* contents,
                int from_index,
                int to_index) override;
  void TabClosingAt(TabStripModel* tab_strip_model,
                    content::WebContents* contents,
                    int index) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;
  void TabStripEmpty() override;
  void TabSelectionChanged(TabStripModel* tab_strip_model,
                           const ui::ListSelectionModel& old_model) override;
  void TabReplacedAt(TabStripModel* tab_strip_model,
                     content::WebContents* old_contents,
                     content::WebContents* new_contents,
                     int index) override;

  // content::WebContentsDelegate:
  void RegisterProtocolHandler(content::WebContents* web_contents,
                               const std::string& protocol,
                               const GURL& url,
                               bool user_gesture) override;
  void UnregisterProtocolHandler(content::WebContents* web_contents,
                                 const std::string& protocol,
                                 const GURL& url,
                                 bool user_gesture) override;
  bool DidAddMessageToConsole(content::WebContents* source,
                              int32_t level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override;
  bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      content::RenderFrameHost* opener,
      content::SiteInstance* source_site_instance,
      int32_t route_id,
      int32_t main_frame_route_id,
      int32_t main_frame_widget_route_id,
      content::mojom::WindowContainerType window_container_type,
      const GURL& opener_url,
      const std::string& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;
  void WebContentsCreated(
      content::WebContents* source_contents,
      int opener_render_process_id,
      int opener_render_frame_id,
      const std::string& frame_name,
      const GURL& target_url,
      content::WebContents* new_contents)
      override;
  void AddNewContents(content::WebContents* source,
                      content::WebContents* new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;
  bool ShouldResumeRequestsForCreatedWindow() override;
  bool IsAttached();
  bool IsPinned() const;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  void BeforeUnloadFired(content::WebContents* tab,
                         bool proceed,
                         bool* proceed_to_fire_unload) override;
  void MoveContents(content::WebContents* source,
                    const gfx::Rect& pos) override;
  void CloseContents(content::WebContents* source) override;
  void ActivateContents(content::WebContents* contents) override;
  void UpdateTargetURL(content::WebContents* source, const GURL& url) override;
  void LoadProgressChanged(content::WebContents* source,
                                   double progress) override;
  bool IsPopupOrPanel(const content::WebContents* source) const override;
  void HandleKeyboardEvent(
      content::WebContents* source,
      const content::NativeWebKeyboardEvent& event) override;
  void EnterFullscreenModeForTab(content::WebContents* source,
                                 const GURL& origin) override;
  void ExitFullscreenModeForTab(content::WebContents* source) override;
  void ContentsZoomChange(bool zoom_in) override;
  void RendererUnresponsive(
      content::WebContents* source,
      const content::WebContentsUnresponsiveState& unresponsive_state) override;
  void RendererResponsive(content::WebContents* source) override;
  bool OnContextMenuShow();
  void OnContextMenuClose();
  bool HandleContextMenu(const content::ContextMenuParams& params) override;
  bool OnGoToEntryOffset(int offset) override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  bool CheckMediaAccessPermission(
      content::WebContents* web_contents,
      const GURL& security_origin,
      content::MediaStreamType type) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override;
  void RequestToLockMouse(
      content::WebContents* web_contents,
      bool user_gesture,
      bool last_unlocked_by_target) override;
  std::unique_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::RenderFrameHost* frame,
      const content::BluetoothChooser::EventHandler& handler) override;
  void UpdatePreferredSize(content::WebContents* web_contents,
                                 const gfx::Size& pref_size) override;

  // content::WebContentsObserver:
  void BeforeUnloadFired(const base::TimeTicks& proceed_time) override;
  void RenderViewReady() override;
  void RenderViewDeleted(content::RenderViewHost*) override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DocumentAvailableInMainFrame() override;
  void DocumentOnLoadCompletedInMainFrame() override;
  void DocumentLoadedInFrame(
      content::RenderFrameHost* render_frame_host) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  void DidFailLoad(content::RenderFrameHost* render_frame_host,
                   const GURL& validated_url,
                   int error_code,
                   const base::string16& error_description) override;
  void DidGetResourceResponseStart(
      const content::ResourceRequestDetails& details) override;
  void DidStartLoading() override;
  void DidStopLoading() override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  bool OnMessageReceived(const IPC::Message& message,
                         content::RenderFrameHost* render_frame_host) override;
  void WebContentsDestroyed() override;
  void NavigationEntryCommitted(
      const content::LoadCommittedDetails& load_details) override;
  void DidChangeVisibleSecurityState() override;
  void TitleWasSet(content::NavigationEntry* entry) override;
  void DidUpdateFaviconURL(
      const std::vector<content::FaviconURL>& urls) override;
  void PluginCrashed(const base::FilePath& plugin_path,
                     base::ProcessId plugin_pid) override;
  void MediaStartedPlaying(const MediaPlayerInfo& media_info,
                           const MediaPlayerId& id) override;
  void MediaStoppedPlaying(const MediaPlayerInfo& media_info,
                           const MediaPlayerId& id,
                           WebContentsObserver::MediaStoppedReason reason)
                           override;
  void DidChangeThemeColor(SkColor theme_color) override;
  void RenderViewCreated(content::RenderViewHost* render_view_host) override;

  // brightray::InspectableWebContentsDelegate:
  void DevToolsReloadPage() override;

  // brightray::InspectableWebContentsViewDelegate:
  void DevToolsFocused() override;
  void DevToolsOpened() override;
  void DevToolsClosed() override;

  void OnRegisterProtocol(content::BrowserContext* browser_context,
      const ProtocolHandler &handler,
      bool allowed);

  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level);

  bool IsBackgroundPage();
  v8::Local<v8::Value> TabValue();

 private:
  friend brave::TabViewGuest;
  friend struct FrameDispatchHelper;

  bool SendIPCSharedMemoryInternal(const base::string16& channel,
                                   base::SharedMemory* shared_memory);
  bool SendIPCMessageInternal(const base::string16& channel,
                              const base::ListValue& args);

  AtomBrowserContext* GetBrowserContext() const;

  uint32_t GetNextRequestId() {
    return ++request_id_;
  }

  // Called when we receive a CursorChange message from chromium.
  void OnCursorChange(const content::WebCursor& cursor);

  // Called when received a message from renderer.
  void OnRendererMessage(content::RenderFrameHost* sender,
                         const base::string16& channel,
                         const base::ListValue& args);

  // Called when received a synchronous message from renderer.
  void OnRendererMessageSync(content::RenderFrameHost* render_frame_host,
                             const base::string16& channel,
                             const base::ListValue& args,
                             IPC::Message* message);

  void OnRendererMessageShared(content::RenderFrameHost* sender,
                               const base::string16& channel,
                               const base::SharedMemoryHandle& shared_memory);

  v8::Global<v8::Value> session_;
  v8::Global<v8::Value> devtools_web_contents_;
  v8::Global<v8::Value> debugger_;

  // The type of current WebContents.
  Type type_;

  // Request id used for findInPage request.
  uint32_t request_id_;

  // Whether to enable devtools.
  bool enable_devtools_;

  bool is_being_destroyed_;

  guest_view::GuestViewBase* guest_delegate_;  // not owned

  // the context menu params for the current context menu;
  content::ContextMenuParams context_menu_params_;

  base::WeakPtrFactory<WebContents> weak_ptr_factory_;

  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;
  DISALLOW_COPY_AND_ASSIGN(WebContents);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_H_
