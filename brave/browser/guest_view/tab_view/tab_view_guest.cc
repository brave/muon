// Copyright 2014 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/guest_view/tab_view/tab_view_guest.h"

#include <stddef.h>

#include <utility>

#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/event.h"
#include "atom/browser/extensions/tab_helper.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "components/guest_view/browser/guest_view_event.h"
#include "components/guest_view/browser/guest_view_manager.h"
#include "components/guest_view/common/guest_view_constants.h"

#include "atom/browser/native_window.h"
#include "atom/browser/web_contents_preferences.h"
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
#include "native_mate/dictionary.h"

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

void TabViewGuest::WebContentsCreated(WebContents* source_contents,
                                      int opener_render_frame_id,
                                      const std::string& frame_name,
                                      const GURL& target_url,
                                      WebContents* new_contents) {
  auto* guest = TabViewGuest::FromWebContents(new_contents);
  CHECK(guest);
  guest->SetOpener(this);
  guest->name_ = frame_name;
  pending_new_windows_.insert(
      std::make_pair(guest, NewWindowInfo(target_url, frame_name)));
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
      new_window_info.changed = new_window_info.url != info.url;
      it->second = new_window_info;
    }
    return nullptr;
  }

  // let the api_web_contents navigate
  return web_contents();
}

void TabViewGuest::DidInitialize(const base::DictionaryValue& create_params) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  api_web_contents_ = atom::api::WebContents::CreateFrom(isolate,
      web_contents()).get();
  web_contents()->SetDelegate(api_web_contents_);
}

content::WebContents* TabViewGuest::CreateNewGuestWindow(
    const content::WebContents::CreateParams& create_params) {
  v8::Isolate* isolate = api_web_contents_->isolate();
  v8::HandleScope handle_scope(isolate);

  content::WebContents::CreateParams params(create_params);
  // window options will come from features that needs to be passed through
  mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);
  options.Set("isGuest", true);

  if (params.browser_context) {
    auto session = atom::api::Session::CreateFrom(isolate,
            Profile::FromBrowserContext(params.browser_context));
    options.Set("session", session);
  }

  auto guest = Create(owner_web_contents());
  params.guest_delegate = guest;

  mate::Handle<atom::api::WebContents> new_api_web_contents =
      atom::api::WebContents::CreateWithParams(isolate, options, params);
  content::WebContents* guest_web_contents =
      new_api_web_contents->GetWebContents();
  guest->InitWithWebContents(base::DictionaryValue(), guest_web_contents);

  return guest_web_contents;
}


void TabViewGuest::CreateWebContents(
    const base::DictionaryValue& create_params,
    const WebContentsCreatedCallback& callback) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  // window options will come from features that needs to be passed through
  mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);
  options.Set("isGuest", true);
  std::string partition;
  create_params.GetString("partition", &partition);
  std::string src;
  create_params.GetString("src", &src);
  options.Set("delayedLoadUrl", src);

  auto session = atom::api::Session::FromPartition(isolate, partition);
  options.Set("session", session);

  auto site_instance = content::SiteInstance::CreateForURL(
        session->browser_context(), GURL(src));
  auto browser_context = session->browser_context();
  content::WebContents::CreateParams
      params(browser_context, site_instance);
  params.guest_delegate = this;

  mate::Handle<atom::api::WebContents> new_api_web_contents =
      atom::api::WebContents::CreateWithParams(isolate, options, params);
  content::WebContents* web_contents = new_api_web_contents->GetWebContents();

  callback.Run(web_contents);
}

void TabViewGuest::DidAttachToEmbedder() {
  api_web_contents_->Emit("did-attach");

  if (GetOpener()) {
    // We need to do a navigation here if the target URL has changed between
    // the time the WebContents was created and the time it was attached.
    // We also need to do an initial navigation if a RenderView was never
    // created for the new window in cases where there is no referrer.
    auto it = GetOpener()->pending_new_windows_.find(this);
    if (it != GetOpener()->pending_new_windows_.end()) {
      const NewWindowInfo& new_window_info = it->second;
      if (new_window_info.changed || !web_contents()->HasOpener()) {
        content::OpenURLParams params(
          new_window_info.url, content::Referrer(), CURRENT_TAB,
          ui::PAGE_TRANSITION_LINK, false);
        api_web_contents_->OpenURLFromTab(web_contents(), params);
      }
      // Once a new guest is attached to the DOM of the embedder page, then the
      // lifetime of the new guest is no longer managed by the opener guest.
      GetOpener()->pending_new_windows_.erase(this);
    }
  }
  api_web_contents_->ResumeLoadingCreatedWebContents();
  web_contents()->WasHidden();
  web_contents()->WasShown();
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
  // we don't use guest only processes
  CHECK(!web_contents()->GetRenderProcessHost()->IsForGuestsOnly());

  web_contents()
      ->GetRenderViewHost()
      ->GetWidget()
      ->GetView()
      ->SetBackgroundColorToDefault();
}

void TabViewGuest::WillDestroy() {
  api_web_contents_->WebContentsDestroyed();
}

void TabViewGuest::GuestSizeChangedDueToAutoSize(const gfx::Size& old_size,
                                                 const gfx::Size& new_size) {
  api_web_contents_->Emit("size-changed",
                          old_size.width(), old_size.height(),
                          new_size.width(), new_size.height());
}

bool TabViewGuest::IsAutoSizeSupported() const {
  return true;
}

TabViewGuest::TabViewGuest(WebContents* owner_web_contents)
    : GuestView<TabViewGuest>(owner_web_contents) {
}

TabViewGuest::~TabViewGuest() {
}

void TabViewGuest::WillAttachToEmbedder() {
  // register the guest for event forwarding
  api_web_contents_->Emit("ELECTRON_GUEST_VIEW_MANAGER_REGISTER_GUEST", 
      extensions::TabHelper::IdForTab(web_contents()));

  // update the owner window
  auto relay = atom::NativeWindowRelay::FromWebContents(
      embedder_web_contents());
  if (relay) {
    api_web_contents_->SetOwnerWindow(relay->window.get());
  }
}

}  // namespace brave
