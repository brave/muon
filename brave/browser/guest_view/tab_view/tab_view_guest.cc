// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <string>
#include <utility>

#include "brave/browser/guest_view/tab_view/tab_view_guest.h"

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/event.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/extensions/tab_helper.h"
#include "base/memory/ptr_util.h"
#include "brave/browser/brave_browser_context.h"
#include "build/build_config.h"
#include "components/guest_view/browser/guest_view_event.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/common/guest_view_constants.h"
#include "extensions/browser/guest_view/web_view/web_view_constants.h"

#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/guest_host.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/site_instance.h"
#include "native_mate/dictionary.h"
#include "ui/base/page_transition_types.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/extensions/atom_browser_client_extensions_part.h"
using extensions::AtomBrowserClientExtensionsPart;
#endif

using content::RenderFrameHost;
using content::RenderProcessHost;
using content::WebContents;
using guest_view::GuestViewBase;
using guest_view::GuestViewEvent;
using guest_view::GuestViewManager;

namespace brave {

// static
GuestViewBase* TabViewGuest::Create(WebContents* owner_web_contents) {
  return new TabViewGuest(owner_web_contents);
}

// static
const char TabViewGuest::Type[] = "webview";

bool TabViewGuest::CanRunInDetachedState() const {
  return true;
}

void TabViewGuest::GuestDestroyed() {
  if (api_web_contents_) {
    api_web_contents_->guest_delegate_ = nullptr;
  }
}

void TabViewGuest::WebContentsCreated(WebContents* source_contents,
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
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // if the browser instance changes the old window won't be able to
  // load the url so let ApplyAttributes handle it
  bool changed =
      AtomBrowserClientExtensionsPart::ShouldSwapBrowsingInstancesForNavigation(
          source_contents->GetSiteInstance(),
          source_contents->GetURL(),
          target_url);
  new_window_info.changed = changed;
#endif
  pending_new_windows_.insert(std::make_pair(guest, new_window_info));
}

WebContents* TabViewGuest::OpenURLFromTab(
    WebContents* source,
    const content::OpenURLParams& params) {
  if (!attached()) {
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

void TabViewGuest::NavigateGuest(const std::string& src,
                                 bool force_navigation) {
  if (src.empty())
    return;

  LoadURLWithParams(GURL(src), content::Referrer(),
      ui::PAGE_TRANSITION_AUTO_TOPLEVEL, force_navigation);
}

void TabViewGuest::DidCommitProvisionalLoadForFrame(
    content::RenderFrameHost* render_frame_host,
    const GURL& url,
    ui::PageTransition transition_type) {
  if (!render_frame_host->GetParent()) {
    src_ = web_contents()->GetURL();
  }

  std::unique_ptr<base::DictionaryValue> args(new base::DictionaryValue());
  args->SetString(guest_view::kUrl, src_.spec());
  args->SetBoolean(guest_view::kIsTopLevel, (!render_frame_host->GetParent() &&
                    ui::PageTransitionCoreTypeIs(
                      transition_type,
                      ui::PAGE_TRANSITION_AUTO_TOPLEVEL)));
  args->SetInteger(webview::kInternalCurrentEntryIndex,
                   web_contents()->GetController().GetCurrentEntryIndex());
  args->SetInteger(webview::kInternalEntryCount,
                   web_contents()->GetController().GetEntryCount());
  args->SetInteger(webview::kInternalProcessId,
                   web_contents()->GetRenderProcessHost()->GetID());
  DispatchEventToView(base::MakeUnique<GuestViewEvent>(
      webview::kEventLoadCommit, std::move(args)));

  // find_helper_.CancelAllFindSessions();
}

void TabViewGuest::DidInitialize(const base::DictionaryValue& create_params) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  api_web_contents_ = atom::api::WebContents::CreateFrom(isolate,
      web_contents(), atom::api::WebContents::Type::WEB_VIEW).get();
  api_web_contents_->guest_delegate_ = this;
  web_contents()->SetDelegate(api_web_contents_);

  ApplyAttributes(create_params);
}

void TabViewGuest::CreateWebContents(
    const base::DictionaryValue& params,
    const WebContentsCreatedCallback& callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  scoped_refptr<atom::AtomBrowserContext> browser_context;

  std::string partition;
  params.GetString("partition", &partition);
  base::DictionaryValue partition_options;
  browser_context =
      brave::BraveBrowserContext::FromPartition(partition, partition_options);
  content::WebContents::CreateParams create_params(browser_context.get());
  create_params.guest_delegate = this;

  mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);

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
      if (attached()) {
        if (new_window_info.changed || !web_contents()->HasOpener()) {
          NavigateGuest(
              new_window_info.url.spec(), false /* force_navigation */);
        }

        // Once a new guest is attached to the DOM of the embedder page the
        // lifetime of the new guest is no longer managed by the opener guest.
        GetOpener()->pending_new_windows_.erase(this);
      }
      is_pending_new_window = true;
    } else if (attached() && clone_) {
      clone_ = false;
      web_contents()->GetController().CopyStateFrom(
          GetOpener()->web_contents()->GetController());
    }
  }

  // handle navigation for src attribute changes
  if (!is_pending_new_window) {
    bool clone = false;
    if (params.GetBoolean("clone", &clone) && clone) {
      clone_ = true;
    } else {
      std::string src;
      if (params.GetString("src", &src)) {
        src_ = GURL(src);
      }
      if (attached()) {
        NavigateGuest(src_.spec(), true);
      }
    }
  }
}

void TabViewGuest::DidAttachToEmbedder() {
  DCHECK(api_web_contents_);

  api_web_contents_->ResumeLoadingCreatedWebContents();

  web_contents()->WasHidden();
  web_contents()->WasShown();

  ApplyAttributes(*attach_params());

  if (web_contents()->GetController().IsInitialNavigation()) {
    web_contents()->GetController().LoadIfNecessary();
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  api_web_contents_->Emit("did-attach",
      extensions::TabHelper::IdForTab(web_contents()));
#else
  api_web_contents_->Emit("did-attach",
      web_contents()->GetRenderProcessHost()->GetID());
#endif
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
  CHECK(!web_contents()->GetRenderProcessHost()->IsForGuestsOnly());

  web_contents()
      ->GetRenderViewHost()
      ->GetWidget()
      ->GetView()
      ->SetBackgroundColorToDefault();
}

void TabViewGuest::WillDestroy() {
  if (api_web_contents_)
    api_web_contents_->WebContentsDestroyed();
  api_web_contents_ = nullptr;

  if (!attached() && GetOpener())
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
      clone_(false) {
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

  if (owner_window)
    api_web_contents_->SetOwnerWindow(web_contents(), owner_window);
}

}  // namespace brave
