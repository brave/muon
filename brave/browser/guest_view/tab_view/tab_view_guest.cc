// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/guest_view/tab_view/tab_view_guest.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/api/event.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
#include "atom/browser/extensions/tab_helper.h"
#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/browser/extensions/api/atom_extensions_api_client.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/brave_browser_context.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/guest_view/browser/guest_view_event.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/guest_host.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_dispatcher_host.h"
#include "content/public/browser/site_instance.h"
#include "extensions/browser/guest_view/web_view/web_view_constants.h"
#include "native_mate/dictionary.h"
#include "ui/base/page_transition_types.h"

using content::RenderFrameHost;
using content::RenderProcessHost;
using content::WebContents;
using extensions::AtomBrowserClientExtensionsPart;
using guest_view::GuestViewBase;
using guest_view::GuestViewEvent;
using guest_view::GuestViewManager;

namespace brave {

namespace {

bool HasWindow(WebContents* web_contents) {
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents);
  return (tab_helper && tab_helper->window_id() != -1);

}

}
// static
GuestViewBase* TabViewGuest::Create(WebContents* owner_web_contents) {
  return new TabViewGuest(owner_web_contents);
}

// static
const char TabViewGuest::Type[] = "webview";

void TabViewGuest::SetCanRunInDetachedState(bool can_run_detached) {
  can_run_detached_ = can_run_detached;
  if (!can_run_detached_ && !attached())
    Destroy(true);
}

bool TabViewGuest::ShouldDestroyOnDetach() const {
  return !can_run_detached_;
}

void TabViewGuest::GuestDestroyed() {
  if (api_web_contents_) {
    api_web_contents_->guest_delegate_ = nullptr;
  }
}

void TabViewGuest::WebContentsCreated(
    WebContents* source_contents,
    int opener_render_process_id,
    int opener_render_frame_id,
    const std::string& frame_name,
    const GURL& target_url,
    WebContents* new_contents) {
  CHECK_EQ(source_contents, web_contents());

  auto* guest = TabViewGuest::FromWebContents(new_contents);
  CHECK(guest);

  guest->SetOpener(this);
  guest->name_ = frame_name;

  NewWindowInfo new_window_info(target_url, frame_name);
  // if the browser instance changes the old window won't be able to
  // load the url so let ApplyAttributes handle it
  bool changed =
      AtomBrowserClientExtensionsPart::ShouldSwapBrowsingInstancesForNavigation(
          source_contents->GetSiteInstance(),
          source_contents->GetURL(),
          target_url);
  new_window_info.changed = changed;
  pending_new_windows_.insert(std::make_pair(guest, new_window_info));
}

WebContents* TabViewGuest::OpenURLFromTab(
    WebContents* source,
    const content::OpenURLParams& params) {
  if (!HasWindow(web_contents())) {
    TabViewGuest* opener = GetOpener();
    // If the guest wishes to navigate away prior to attachment then we save the
    // navigation to perform upon attachment. Navigation initializes a lot of
    // state that assumes an embedder exists, such as RenderWidgetHostViewGuest.
    // Navigation also resumes resource loading. If we were created using
    // newwindow (i.e. we have an opener), we don't allow navigation until
    // attachment.
    if (opener) {
      auto it = opener->pending_new_windows_.find(this);
      if (it == opener->pending_new_windows_.end())
        return nullptr;
      const NewWindowInfo& info = it->second;
      NewWindowInfo new_window_info(params.url, info.name);
      new_window_info.changed = info.changed || new_window_info.url != info.url;
      it->second = new_window_info;
      return nullptr;
    }
  }

  // let the api_web_contents navigate
  return web_contents();
}

void TabViewGuest::LoadURLWithParams(
    const GURL& url,
    const content::Referrer& referrer,
    ui::PageTransition transition_type,
    bool force_navigation) {
  if (!force_navigation && (src_ == url))
    return;

  GURL validated_url(url);

  content::NavigationController::LoadURLParams load_url_params(validated_url);
  load_url_params.referrer = referrer;
  load_url_params.transition_type = transition_type;
  GuestViewBase::LoadURLWithParams(load_url_params);

  src_ = validated_url;
}

void TabViewGuest::Load() {
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (!tab_helper || !tab_helper->IsDiscarded()) {
    api_web_contents_->ResumeLoadingCreatedWebContents();

    web_contents()->WasHidden();
    web_contents()->WasShown();
  }

  ApplyAttributes(*attach_params());
}

void TabViewGuest::NavigateGuest(const std::string& src,
                                 bool force_navigation) {
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (src.empty() || (tab_helper && tab_helper->IsDiscarded()))
    return;

  LoadURLWithParams(GURL(src), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, force_navigation);
}

void TabViewGuest::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsInMainFrame()) {
    src_ = web_contents()->GetURL();
  }

  src_ = web_contents()->GetURL();
  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  args->SetString(guest_view::kUrl, src_.spec());
  args->SetBoolean(guest_view::kIsTopLevel, navigation_handle->IsInMainFrame());
  args->SetInteger(webview::kInternalCurrentEntryIndex,
                   web_contents()->GetController().GetCurrentEntryIndex());
  args->SetInteger(webview::kInternalEntryCount,
                   web_contents()->GetController().GetEntryCount());
  args->SetInteger(webview::kInternalProcessId,
                   web_contents()->GetMainFrame()->GetProcess()->GetID());
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(
      webview::kEventLoadCommit, std::move(args)));

  // find_helper_.CancelAllFindSessions();
}

void TabViewGuest::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // loadStart shouldn't be sent for same page navigations.
  if (navigation_handle->IsSameDocument() ||
      !navigation_handle->IsInMainFrame())
    return;

  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  args->SetString(guest_view::kUrl, navigation_handle->GetURL().spec());
  args->SetBoolean(guest_view::kIsTopLevel, navigation_handle->IsInMainFrame());
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(webview::kEventLoadStart,
                                                       std::move(args)));
}

void TabViewGuest::AttachGuest(int guest_instance_id) {
  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  args->SetInteger("guestInstanceId", guest_instance_id);
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(
      "webViewInternal.onAttachGuest", std::move(args)));
}

void TabViewGuest::DetachGuest() {
  api_web_contents_->Emit("will-detach",
      extensions::TabHelper::IdForTab(web_contents()));
  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(
      "webViewInternal.onDetachGuest", std::move(args)));
}

void TabViewGuest::TabIdChanged() {
  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  args->SetInteger("tabID", extensions::TabHelper::IdForTab(web_contents()));
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(
      "webViewInternal.onTabIdChanged", std::move(args)));
}

void TabViewGuest::DidInitialize(const base::DictionaryValue& create_params) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  api_web_contents_ = atom::api::WebContents::CreateFrom(isolate,
      web_contents(), atom::api::WebContents::Type::WEB_VIEW).get();
  api_web_contents_->guest_delegate_ = this;
  web_contents()->SetDelegate(api_web_contents_);

  if (!attach_params()) {
    SetAttachParams(create_params);
  }
}

void TabViewGuest::CreateWebContents(
    const base::DictionaryValue& params,
    const WebContentsCreatedCallback& callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);

  std::string src;
  if (params.GetString("src", &src)) {
    src_ = GURL(src);
  }
  std::string name;
  if (params.GetString("name", &name))
    options.Set("name", name);
  std::string partition;
  params.GetString("partition", &partition);
  base::DictionaryValue partition_options;
  std::string parent_partition;
  if (params.GetString("parent_partition", &parent_partition)) {
    partition_options.SetString("parent_partition", parent_partition);
  } else {
    std::string session_tab_prefix = "persist:partition-";
    std::string private_tab_prefix = "partition-";
    if (partition.compare(0, session_tab_prefix.length(),
          session_tab_prefix) == 0 ||
       partition.compare(0, private_tab_prefix.length(),
          private_tab_prefix) == 0) {
      partition_options.SetString("parent_partition", "");
    }
  }
  atom::AtomBrowserContext* browser_context =
      brave::BraveBrowserContext::FromPartition(partition, partition_options);

  content::WebContents::CreateParams create_params(browser_context);
  create_params.guest_delegate = this;

  mate::Handle<atom::api::WebContents> new_api_web_contents =
      atom::api::WebContents::CreateWithParams(isolate, options, create_params);

  content::WebContents* web_contents = new_api_web_contents->web_contents();
  callback.Run(web_contents);
}

void TabViewGuest::ApplyAttributes(const base::DictionaryValue& params) {
  bool is_pending_new_window = false;
  if (GetOpener()) {
    // We need to do a navigation here if the target URL has changed between
    // the time the WebContents was created and the time it was attached.
    // We also need to do an initial navigation if a RenderView was never
    // created for the new window in cases where there is no referrer.
    auto it = GetOpener()->pending_new_windows_.find(this);
    if (it != GetOpener()->pending_new_windows_.end()) {
      const NewWindowInfo& new_window_info = it->second;
      if (HasWindow(web_contents())) {
        if (new_window_info.changed || !web_contents()->HasOpener()) {
          NavigateGuest(
              new_window_info.url.spec(), false /* force_navigation */);
        }

        // Once a new guest is attached to the DOM of the embedder page the
        // lifetime of the new guest is no longer managed by the opener guest.
        GetOpener()->pending_new_windows_.erase(this);
      }
      is_pending_new_window = true;
    }
  }

  // handle navigation for src attribute changes
  if (!is_pending_new_window) {
    std::string src;
    if (params.GetString("src", &src)) {
      src_ = GURL(src);
    }

    if (web_contents()->GetController().IsInitialNavigation()) {
      if (web_contents()->GetController().NeedsReload()) {
        web_contents()->GetController().LoadIfNecessary();
        return;
      }

      // don't reload if we're already loading
      if (web_contents()->GetController().GetPendingEntry() &&
          web_contents()->GetController().GetPendingEntry()->GetURL() ==
          src_) {
        return;
      }
      NavigateGuest(src_.spec(), true);
    }
  }

  bool allow_transparency = false;
  if (params.GetBoolean(webview::kAttributeAllowTransparency,
      &allow_transparency)) {
    // We need to set the background opaque flag after navigation to ensure that
    // there is a RenderWidgetHostView available.
    SetAllowTransparency(allow_transparency);
  }
}

void TabViewGuest::SetAllowTransparency(bool allow) {
  if (allow_transparency_ == allow)
    return;

  allow_transparency_ = allow;
  auto* rvh = web_contents()->GetRenderViewHost();
  if (!rvh || !rvh->GetWidget()->GetView())
    return;

  SetTransparency();
}

void TabViewGuest::SetTransparency() {
  auto* rvh = web_contents()->GetRenderViewHost();
  if (!rvh)
    return;
  
  auto* view = rvh->GetWidget()->GetView();
  if (!view)
    return;
  
  if (allow_transparency_)
    view->SetBackgroundColor(SK_ColorTRANSPARENT);
  else
    view->SetBackgroundColorToDefault();
}

void TabViewGuest::DidAttachToEmbedder() {
  DCHECK(api_web_contents_);

  ApplyAttributes(*attach_params());

  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (tab_helper)
    tab_helper->DidAttach();

  api_web_contents_->Emit("did-attach",
      extensions::TabHelper::IdForTab(web_contents()));
}

void TabViewGuest::DidDetachFromEmbedder() {
  if (api_web_contents_) {
    api_web_contents_->Emit("did-detach",
        extensions::TabHelper::IdForTab(web_contents()));
  }
}

bool TabViewGuest::ZoomPropagatesFromEmbedderToGuest() const {
  return false;
}

const char* TabViewGuest::GetAPINamespace() const {
  return "webViewInternal";
}

int TabViewGuest::GetTaskPrefix() const {
  // return IDS_EXTENSION_TASK_MANAGER_WEBVIEW_TAG_PREFIX;
  return -1;
}

void TabViewGuest::GuestReady() {
  // we don't use guest only processes and don't want those limitations
  CHECK(!web_contents()->GetMainFrame()->GetProcess()->IsForGuestsOnly());

  api_web_contents_->Emit("guest-ready",
      extensions::TabHelper::IdForTab(web_contents()), guest_instance_id());

  // We don't want to accidentally set the opacity of an interstitial page.
  // WebContents::GetRenderWidgetHostView will return the RWHV of an
  // interstitial page if one is showing at this time. We only want opacity
  // to apply to web pages.
  SetTransparency();
}

void TabViewGuest::WillDestroy() {
  api_web_contents_ = nullptr;

  if (GetOpener())
    GetOpener()->pending_new_windows_.erase(this);
}

void TabViewGuest::GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                                 const gfx::Size& new_size) {
  if (api_web_contents_)
    api_web_contents_->Emit("size-changed",
                            old_size.width(), old_size.height(),
                            new_size.width(), new_size.height());
}

bool TabViewGuest::IsAutoSizeSupported() const {
  return true;
}

TabViewGuest::TabViewGuest(WebContents* owner_web_contents)
    : GuestView<TabViewGuest>(owner_web_contents),
      api_web_contents_(nullptr),
      can_run_detached_(true),
      allow_transparency_(false) {
}

TabViewGuest::~TabViewGuest() {
}

void TabViewGuest::WillAttachToEmbedder() {
  DCHECK(api_web_contents_);
  api_web_contents_->Emit("will-attach", owner_web_contents());

  // update the owner window
  atom::NativeWindow* owner_window = nullptr;
  auto relay = atom::NativeWindowRelay::FromWebContents(
      owner_web_contents());
  if (relay)
    owner_window = relay->window.get();

  if (owner_window) {
    auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
    if (tab_helper) {
      TabIdChanged();
      tab_helper->SetBrowser(owner_window->browser());
    }
    api_web_contents_->SetOwnerWindow(owner_window);
  }
}

}  // namespace brave
