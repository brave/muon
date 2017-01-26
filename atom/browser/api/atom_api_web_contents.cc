// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <memory>
#include <set>
#include <string>
#include <utility>

#include "atom/browser/api/atom_api_web_contents.h"

#include "atom/browser/api/atom_api_debugger.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/atom_api_web_request.h"
#include "atom/browser/api/atom_api_window.h"
#include "atom/browser/api/event.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/atom_security_state_model_client.h"
#include "atom/browser/autofill/atom_autofill_client.h"
#include "atom/browser/browser.h"
#include "atom/browser/lib/bluetooth_chooser.h"
#include "atom/browser/native_window.h"
#include "atom/browser/net/atom_network_delegate.h"
#include "atom/browser/ui/drag_util.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/api/api_messages.h"
#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/color_util.h"
#include "atom/common/mouse_util.h"
#include "atom/common/native_mate_converters/blink_converter.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/content_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gfx_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/string16_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/options_switches.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/browser/brave_browser_context.h"
#include "brave/browser/brave_content_browser_client.h"
#include "brave/browser/guest_view/tab_view/tab_view_guest.h"
#include "brave/browser/plugins/brave_plugin_service_filter.h"
#include "brave/browser/renderer_preferences_helper.h"
#include "brightray/browser/inspectable_web_contents.h"
#include "brightray/browser/inspectable_web_contents_view.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry.h"
#include "chrome/browser/custom_handlers/protocol_handler_registry_factory.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "components/autofill/content/browser/content_autofill_driver_factory.h"
#include "components/autofill/core/browser/autofill_manager.h"
#include "components/zoom/page_zoom.h"
#include "components/zoom/zoom_controller.h"
#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/public/browser/browser_plugin_guest_manager.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/memory_pressure_controller.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_request_details.h"
#include "content/public/browser/security_style_explanations.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/context_menu_params.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request_context.h"
#include "printing/print_settings.h"
#include "third_party/WebKit/public/platform/WebInputEvent.h"
#include "third_party/WebKit/public/web/WebFindOptions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/screen.h"

#if !defined(OS_MACOSX)
#include "ui/aura/window.h"
#endif

#include "atom/common/node_includes.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "atom/browser/api/atom_api_extension.h"
#include "atom/browser/extensions/tab_helper.h"
#include "extensions/browser/api/extensions_api_client.h"
#endif

using extensions::ExtensionsAPIClient;
using guest_view::GuestViewManager;

namespace mate {

template<>
struct Converter<atom::SetSizeParams> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     atom::SetSizeParams* out) {
    mate::Dictionary params;
    if (!ConvertFromV8(isolate, val, &params))
      return false;
    bool autosize;
    if (params.Get("enableAutoSize", &autosize))
      out->enable_auto_size.reset(new bool(true));
    gfx::Size size;
    if (params.Get("min", &size))
      out->min_size.reset(new gfx::Size(size));
    if (params.Get("max", &size))
      out->max_size.reset(new gfx::Size(size));
    if (params.Get("normal", &size))
      out->normal_size.reset(new gfx::Size(size));
    return true;
  }
};

template<>
struct Converter<printing::PrintSettings> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     printing::PrintSettings* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    bool silent = false;
    dict.Get("silent", &silent);
    // out->set_should_print_backgrounds(silent);

    bool should_print_backgrounds = false;
    dict.Get("printBackground", &should_print_backgrounds);
    out->set_should_print_backgrounds(should_print_backgrounds);
    return true;
  }
};

template<>
struct Converter<content::SavePageType> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     content::SavePageType* out) {
    std::string save_type;
    if (!ConvertFromV8(isolate, val, &save_type))
      return false;
    save_type = base::ToLowerASCII(save_type);
    if (save_type == "htmlonly") {
      *out = content::SAVE_PAGE_TYPE_AS_ONLY_HTML;
    } else if (save_type == "htmlcomplete") {
      *out = content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML;
    } else if (save_type == "mhtml") {
      *out = content::SAVE_PAGE_TYPE_AS_MHTML;
    } else {
      return false;
    }
    return true;
  }
};

template<>
struct Converter<WindowContainerType> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowContainerType val) {
    std::string type;
    switch (val) {
      case WINDOW_CONTAINER_TYPE_NORMAL:
        type = "normal";
        break;
      case WINDOW_CONTAINER_TYPE_BACKGROUND:
        type = "background";
        break;
      case WINDOW_CONTAINER_TYPE_PERSISTENT:
        type = "persistent";
        break;
      default:
        type = "unknown";
        break;
    }
    return mate::ConvertToV8(isolate, type);
  }
};

template<>
struct Converter<atom::api::WebContents::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   atom::api::WebContents::Type val) {
    using Type = atom::api::WebContents::Type;
    std::string type = "";
    switch (val) {
      case Type::BACKGROUND_PAGE: type = "backgroundPage"; break;
      case Type::BROWSER_WINDOW: type = "window"; break;
      case Type::REMOTE: type = "remote"; break;
      case Type::WEB_VIEW: type = "webview"; break;
      default: break;
    }
    return mate::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     atom::api::WebContents::Type* out) {
    using Type = atom::api::WebContents::Type;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "webview") {
      *out = Type::WEB_VIEW;
    } else if (type == "backgroundPage") {
      *out = Type::BACKGROUND_PAGE;
    } else {
      return false;
    }
    return true;
  }
};

template<>
struct Converter<blink::WebSecurityStyle> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   blink::WebSecurityStyle val) {
    std::string type;
    switch (val) {
      case blink::WebSecurityStyleUnauthenticated:
        type = "insecure";
        break;
      case blink::WebSecurityStyleAuthenticationBroken:
        type = "broken";
        break;
      case blink::WebSecurityStyleWarning:
        type = "warning";
        break;
      case blink::WebSecurityStyleAuthenticated:
        type = "secure";
        break;
      default:
        type = "unknown";
        break;
    }
    return mate::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     blink::WebSecurityStyle* out) {
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "insecure") {
      *out = blink::WebSecurityStyleUnauthenticated;
    } else if (type == "broken") {
      *out = blink::WebSecurityStyleAuthenticationBroken;
    } else if (type == "warning") {
      *out = blink::WebSecurityStyleWarning;
    } else if (type == "secure") {
      *out = blink::WebSecurityStyleAuthenticated;
    } else {
      return false;
    }
    return true;
  }
};

}  // namespace mate


namespace atom {

namespace api {

namespace {

content::ServiceWorkerContext* GetServiceWorkerContext(
    const content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  auto site_instance = web_contents->GetSiteInstance();
  if (!context || !site_instance)
    return nullptr;

  auto storage_partition =
      content::BrowserContext::GetStoragePartition(context, site_instance);
  if (!storage_partition)
    return nullptr;

  return storage_partition->GetServiceWorkerContext();
}

// Called when CapturePage is done.
void OnCapturePageDone(base::Callback<void(const gfx::Image&)> callback,
                       const SkBitmap& bitmap,
                       content::ReadbackResponse response) {
  callback.Run(gfx::Image::CreateFrom1xBitmap(bitmap));
}

}  // namespace

WebContents::WebContents(v8::Isolate* isolate,
                         content::WebContents* web_contents,
                         Type type)
    : content::WebContentsObserver(web_contents),
      type_(type),
      request_id_(0),
      enable_devtools_(true),
      is_being_destroyed_(false),
      guest_delegate_(nullptr) {
  if (type == REMOTE) {
    Init(isolate);
    AttachAsUserData(web_contents);
  } else {
    const mate::Dictionary options = mate::Dictionary::CreateEmpty(isolate);
    auto session = Session::CreateFrom(isolate, GetBrowserContext());
    session_.Reset(isolate, session.ToV8());
    CompleteInit(isolate, web_contents, options);
  }
}

WebContents::WebContents(v8::Isolate* isolate, const mate::Dictionary& options,
    const content::WebContents::CreateParams& create_params)
  : type_(BROWSER_WINDOW),
    request_id_(0),
    enable_devtools_(true),
    is_being_destroyed_(false),
    guest_delegate_(nullptr) {
  CreateWebContents(isolate, options, create_params);
}

WebContents::WebContents(v8::Isolate* isolate, const mate::Dictionary& options)
    : type_(BROWSER_WINDOW),
      request_id_(0),
      enable_devtools_(true),
      is_being_destroyed_(false),
      guest_delegate_(nullptr) {
  mate::Handle<api::Session> session;

  // Obtain the session.
  std::string partition;
  if (options.Get("partition", &session)) {
  } else if (options.Get("partition", &partition)) {
    session = Session::FromPartition(isolate, partition);
  } else {
    // Use the default session if not specified.
    session = Session::FromPartition(isolate, "");
  }
  content::WebContents::CreateParams create_params(session->browser_context());
  CreateWebContents(isolate, options, create_params);
}

WebContents::~WebContents() {
  // The WebContentsDestroyed will not be called automatically because we
  // unsubscribe from webContents before destroying it. So we have to manually
  // call it here to make sure "destroyed" event is emitted.
  WebContentsDestroyed();
}

void WebContents::CreateWebContents(v8::Isolate* isolate,
    const mate::Dictionary& options,
    const content::WebContents::CreateParams& create_params) {
  content::WebContents::CreateParams params(create_params);
  content::BrowserContext* browser_context = params.browser_context;

  auto session = Session::CreateFrom(isolate,
      static_cast<brave::BraveBrowserContext*>(params.browser_context));
  session_.Reset(isolate, session.ToV8());

  bool b = false;
  if (params.guest_delegate) {
    type_ = WEB_VIEW;
  }

  content::WebContents* web_contents = content::WebContents::Create(params);
  CompleteInit(isolate, web_contents, options);
}

void WebContents::CompleteInit(v8::Isolate* isolate,
    content::WebContents *web_contents,
    const mate::Dictionary& options) {
  Observe(web_contents);

  InitWithWebContents(web_contents, GetBrowserContext());

  managed_web_contents()->GetView()->SetDelegate(this);

  // Save the preferences in C++.
  new WebContentsPreferences(web_contents, options);

  // Intialize permission helper.
  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  // Intialize security state client.
  AtomSecurityStateModelClient::CreateForWebContents(web_contents);

  // Intialize permission helper.
  WebContentsPermissionHelper::CreateForWebContents(web_contents);
  // Intialize security state client.
  AtomSecurityStateModelClient::CreateForWebContents(web_contents);

  // Initialize zoom
  zoom::ZoomController::CreateForWebContents(web_contents);
  brave::RendererPreferencesHelper::CreateForWebContents(web_contents);
  // Initialize autofill client
  autofill::AtomAutofillClient::CreateForWebContents(web_contents);
  std::string locale = static_cast<brave::BraveContentBrowserClient*>(
      brave::BraveContentBrowserClient::Get())->GetApplicationLocale();
  autofill::AtomAutofillClient::FromWebContents(web_contents)->Initialize(this);
  autofill::ContentAutofillDriverFactory::CreateForWebContentsAndDelegate(
      web_contents,
      autofill::AtomAutofillClient::FromWebContents(web_contents),
      locale,
      autofill::AutofillManager::DISABLE_AUTOFILL_DOWNLOAD_MANAGER);

  Init(isolate);
  AttachAsUserData(web_contents);

  memory_pressure_listener_.reset(new base::MemoryPressureListener(
      base::Bind(&WebContents::OnMemoryPressure, base::Unretained(this))));
}

bool WebContents::DidAddMessageToConsole(content::WebContents* source,
                                         int32_t level,
                                         const base::string16& message,
                                         int32_t line_no,
                                         const base::string16& source_id) {
  if (type_ == BROWSER_WINDOW) {
    return false;
  } else {
    Emit("console-message", level, message, line_no, source_id);
    return true;
  }
}

bool WebContents::ShouldCreateWebContents(
    content::WebContents* web_contents,
    int32_t route_id,
    int32_t main_frame_route_id,
    int32_t main_frame_widget_route_id,
    WindowContainerType window_container_type,
    const std::string& frame_name,
    const GURL& target_url,
    const std::string& partition_id,
    content::SessionStorageNamespace* session_storage_namespace) {
  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env)
    return false;

  auto event = v8::Local<v8::Object>::Cast(
      mate::Event::Create(isolate()).ToV8());

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "should-create-web-contents",
                  event,
                  web_contents,
                  window_container_type,
                  frame_name,
                  target_url,
                  partition_id);

  return !event->Get(
      mate::StringToV8(isolate(), "defaultPrevented"))->BooleanValue();
}

void WebContents::WebContentsCreated(content::WebContents* source_contents,
                                int opener_render_process_id,
                                int opener_render_frame_id,
                                const std::string& frame_name,
                                const GURL& target_url,
                                content::WebContents* new_contents) {
  if (guest_delegate_) {
    guest_delegate_->WebContentsCreated(source_contents,
                                        opener_render_process_id,
                                        opener_render_frame_id,
                                        frame_name,
                                        target_url,
                                        new_contents);
  }
}

void WebContents::AddNewContents(content::WebContents* source,
                    content::WebContents* new_contents,
                    WindowOpenDisposition disposition,
                    const gfx::Rect& initial_rect,
                    bool user_gesture,
                    bool* was_blocked) {
  if (Extension::IsBackgroundPageWebContents(source)) {
    user_gesture = true;
  }

  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env) {
    return;
  }

  auto event = v8::Local<v8::Object>::Cast(
      mate::Event::Create(isolate()).ToV8());

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "add-new-contents",
                  event,
                  source,
                  new_contents,
                  disposition,
                  initial_rect,
                  user_gesture);
  bool blocked = event->Get(
      mate::StringToV8(isolate(), "defaultPrevented"))->BooleanValue();

  if (was_blocked)
    *was_blocked = blocked;

  if (blocked) {
    delete new_contents;
  }
}

bool WebContents::ShouldResumeRequestsForCreatedWindow() {
  return false;
}

bool WebContents::IsAttached() {
  if (guest_delegate_)
    return guest_delegate_->attached();

  return owner_window() != nullptr;
}

void WebContents::AutofillSelect(const std::string& value,
                                 int frontend_id, int index) {
  auto autofillClient =
    autofill::AtomAutofillClient::FromWebContents(web_contents());
  if (autofillClient)
    autofillClient->DidAcceptSuggestion(value, frontend_id, index);
}

void WebContents::AutofillPopupHidden() {
  auto autofillClient =
    autofill::AtomAutofillClient::FromWebContents(web_contents());
  if (autofillClient)
    autofillClient->PopupHidden();
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  if (guest_delegate_ && !guest_delegate_->OpenURLFromTab(source, params))
    return nullptr;

  if (params.disposition == CURRENT_TAB) {
    CommonWebContentsDelegate::OpenURLFromTab(source, params);
    return source;
  }

  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env)
    return nullptr;

  auto event = v8::Local<v8::Object>::Cast(
      mate::Event::Create(isolate()).ToV8());

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "open-url-from-tab",
                  event,
                  source,
                  params.url,
                  params.disposition);

  return nullptr;
}

void WebContents::BeforeUnloadFired(content::WebContents* tab,
                                    bool proceed,
                                    bool* proceed_to_fire_unload) {
  if (type_ == BROWSER_WINDOW)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
}

void WebContents::MoveContents(content::WebContents* source,
                               const gfx::Rect& pos) {
  Emit("move", pos);
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");

  if ((type_ == BROWSER_WINDOW) && owner_window())
    owner_window()->CloseContents(source);
}

void WebContents::ActivateContents(content::WebContents* source) {
  Emit("activate");
}

void WebContents::UpdateTargetURL(content::WebContents* source,
                                  const GURL& url) {
  const gfx::Point& location =
      display::Screen::GetScreen()->GetCursorScreenPoint();
  const gfx::Rect& bounds = web_contents()->GetContainerBounds();
  int x = location.x() - bounds.x();
  int y = location.y() - bounds.y();
  Emit("update-target-url", url, x, y);
}

void WebContents::LoadProgressChanged(content::WebContents* source,
                                   double progress) {
  Emit("load-progress-changed", progress);
}

bool WebContents::IsPopupOrPanel(const content::WebContents* source) const {
  return type_ == BROWSER_WINDOW;
}

void WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (type_ == WEB_VIEW && HostWebContents()) {
    // Send the unhandled keyboard events back to the embedder.
    auto embedder =
        atom::api::WebContents::CreateFrom(isolate(), HostWebContents());
    embedder->HandleKeyboardEvent(source, event);
  } else {
    // Go to the default keyboard handling.
    CommonWebContentsDelegate::HandleKeyboardEvent(source, event);
  }
}

void WebContents::EnterFullscreenModeForTab(content::WebContents* source,
                                            const GURL& origin) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(source);
  auto callback = base::Bind(&WebContents::OnEnterFullscreenModeForTab,
                             base::Unretained(this), source, origin);
  permission_helper->RequestFullscreenPermission(callback);
}

void WebContents::OnEnterFullscreenModeForTab(content::WebContents* source,
                                              const GURL& origin,
                                              bool allowed) {
  if (!allowed)
    return;
  CommonWebContentsDelegate::EnterFullscreenModeForTab(source, origin);
  Emit("enter-html-full-screen");
}

void WebContents::ExitFullscreenModeForTab(content::WebContents* source) {
  CommonWebContentsDelegate::ExitFullscreenModeForTab(source);
  Emit("leave-html-full-screen");
}

void WebContents::RendererUnresponsive(
    content::WebContents* source
    const content::WebContentsUnresponsiveState& unresponsive_state) {
  Emit("unresponsive");
  if ((type_ == BROWSER_WINDOW) && owner_window())
    owner_window()->RendererUnresponsive(source, unresponsive_state);
}

void WebContents::RendererResponsive(content::WebContents* source) {
  Emit("responsive");
  if ((type_ == BROWSER_WINDOW) && owner_window())
    owner_window()->RendererResponsive(source);
}

bool WebContents::HandleContextMenu(const content::ContextMenuParams& params) {
#if defined(ENABLE_PLUGINS)
    if (params.custom_context.request_id &&
        !params.custom_context.is_pepper_menu) {
      Emit("enable-pepper-menu", std::make_pair(params, web_contents()));
      web_contents()->NotifyContextMenuClosed(params.custom_context);
      return true;
    }
#endif

  if (params.custom_context.is_pepper_menu) {
    Emit("pepper-context-menu", std::make_pair(params, web_contents()));
    web_contents()->NotifyContextMenuClosed(params.custom_context);
  } else {
    Emit("context-menu", std::make_pair(params, web_contents()));
  }

  return true;
}

void WebContents::AuthorizePlugin(mate::Arguments* args) {
  if (args->Length() != 1) {
    args->ThrowError("Wrong number of arguments");
    return;
  }

  std::string resource_id;
  if (!args->GetNext(&resource_id)) {
    args->ThrowError("resourceId is a required field");
    return;
  }

  BravePluginServiceFilter::GetInstance()->AuthorizeAllPlugins(
      web_contents(), true, resource_id);
}

bool WebContents::OnGoToEntryOffset(int offset) {
  GoToOffset(offset);
  return false;
}

void WebContents::FindReply(content::WebContents* web_contents,
                            int request_id,
                            int number_of_matches,
                            const gfx::Rect& selection_rect,
                            int active_match_ordinal,
                            bool final_update) {
  if (!final_update)
    return;

  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  mate::Dictionary result = mate::Dictionary::CreateEmpty(isolate());
  result.Set("requestId", request_id);
  result.Set("matches", number_of_matches);
  result.Set("selectionArea", selection_rect);
  result.Set("activeMatchOrdinal", active_match_ordinal);
  result.Set("finalUpdate", final_update);  // Deprecate after 2.0
  Emit("found-in-page", result);
}

bool WebContents::CheckMediaAccessPermission(
    content::WebContents* web_contents,
    const GURL& security_origin,
    content::MediaStreamType type) {
  return true;
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, callback);
}

void WebContents::RequestToLockMouse(
    content::WebContents* web_contents,
    bool user_gesture,
    bool last_unlocked_by_target) {
  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(user_gesture);
}

std::unique_ptr<content::BluetoothChooser> WebContents::RunBluetoothChooser(
    content::RenderFrameHost* frame,
    const content::BluetoothChooser::EventHandler& event_handler) {
  std::unique_ptr<BluetoothChooser> bluetooth_chooser(
      new BluetoothChooser(this, event_handler));
  return std::move(bluetooth_chooser);
}

void WebContents::EnablePreferredSizeMode(bool enable) {
  web_contents()->GetRenderViewHost()->EnablePreferredSizeMode();
}

void WebContents::UpdatePreferredSize(content::WebContents* web_contents,
                                 const gfx::Size& pref_size) {
  Emit("preferred-size-changed", pref_size);
}

void WebContents::BeforeUnloadFired(const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void WebContents::RenderViewReady() {
  Emit("render-view-ready");
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  Emit("render-view-deleted", render_view_host->GetProcess()->GetID());
}

void WebContents::RenderProcessGone(base::TerminationStatus status) {
  Emit("crashed");
}

void WebContents::PluginCrashed(const base::FilePath& plugin_path,
                                base::ProcessId plugin_pid) {
  content::WebPluginInfo info;
  auto plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  Emit("plugin-crashed", info.name, info.version);
}

void WebContents::MediaStartedPlaying(const MediaPlayerInfo& media_info,
                                      const MediaPlayerId& id) {
  Emit("media-started-playing");
}

void WebContents::MediaStoppedPlaying(const MediaPlayerId& id) {
  Emit("media-paused");
}

void WebContents::DidChangeThemeColor(SkColor theme_color) {
  std::string hex_theme_color = base::StringPrintf("#%02X%02X%02X",
    SkColorGetR(theme_color),
    SkColorGetG(theme_color),
    SkColorGetB(theme_color));
  Emit("did-change-theme-color", hex_theme_color);
}

void WebContents::DocumentAvailableInMainFrame() {
  Emit("document-available");
}

void WebContents::DocumentOnLoadCompletedInMainFrame() {
  Emit("document-onload");
}

void WebContents::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  if (!render_frame_host->GetParent())
    Emit("dom-ready");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  Emit("did-frame-finish-load", is_main_frame);

  if (is_main_frame)
    Emit("did-finish-load", validated_url);
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& url,
                              int error_code,
                              const base::string16& error_description,
                              bool was_ignored_by_handler) {
  bool is_main_frame = !render_frame_host->GetParent();
  Emit("did-fail-load", error_code, error_description, url,
      is_main_frame, was_ignored_by_handler);
}

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
  Emit("did-stop-loading");
}

void WebContents::DidGetResourceResponseStart(
    const content::ResourceRequestDetails& details) {
  Emit("did-get-response-details",
       details.socket_address.IsEmpty(),
       details.url,
       details.original_url,
       details.http_response_code,
       details.method,
       details.referrer,
       details.headers.get(),
       ResourceTypeToString(details.resource_type));
}

void WebContents::DidGetRedirectForResourceRequest(
    const content::ResourceRedirectDetails& details) {
  Emit("did-get-redirect-request",
       details.url,
       details.new_url,
       (details.resource_type == content::RESOURCE_TYPE_MAIN_FRAME),
       details.http_response_code,
       details.method,
       details.referrer,
       details.headers.get());
}

void WebContents::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->HasCommitted() &&
      (navigation_handle->IsSamePage() || navigation_handle->IsErrorPage()))
    return;

  auto url = navigation_handle->GetURL();
  bool is_main_frame = navigation_handle->IsInMainFrame();
  bool is_error_page = navigation_handle->IsErrorPage();
  bool is_src_doc = navigation_handle->IsSrcdoc();
  Emit("load-start", url, is_main_frame, is_error_page, is_src_doc);
}

void WebContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  bool is_main_frame = navigation_handle->IsInMainFrame();
  auto url = navigation_handle->GetURL();
  if (navigation_handle->HasCommitted() && !navigation_handle->IsErrorPage()) {
    bool is_in_page = navigation_handle->IsSamePage();
    bool is_renderer_initiated = navigation_handle->IsRendererInitiated();
    if (!is_in_page) {
      Emit("load-commit", url, is_main_frame);
    }
    if (is_main_frame && !is_in_page) {
      Emit("did-navigate", url, is_renderer_initiated);
    } else if (is_in_page) {
      Emit("did-navigate-in-page", url, is_main_frame);
    }
  } else {
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    Emit("did-fail-provisional-load", code, description, url, is_main_frame);
  }
}

void WebContents::SecurityStyleChanged(
    blink::WebSecurityStyle security_style,
    const content::SecurityStyleExplanations& explanations) {
    if (explanations.displayed_mixed_content &&
        security_style == content::SECURITY_STYLE_UNAUTHENTICATED) {
      Emit("security-style-changed", "passive-mixed-content");
    } else {
      Emit("security-style-changed", security_style);
    }
}

void WebContents::TitleWasSet(content::NavigationEntry* entry,
                              bool explicit_set) {
  if (entry)
    Emit("-page-title-updated", entry->GetTitle(), explicit_set);
  else
    Emit("-page-title-updated", "", explicit_set);
}

void WebContents::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& urls) {
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter.icon_type != content::FaviconURL::FAVICON)
      continue;
    const GURL& url = iter.icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  Emit("page-favicon-updated", unique_urls);
}

void WebContents::DevToolsReloadPage() {
  Emit("devtools-reload-page");
}

void WebContents::DevToolsFocused() {
  Emit("devtools-focused");
}

void WebContents::DevToolsOpened() {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  if (!managed_web_contents() ||
      !managed_web_contents()->GetDevToolsWebContents())
    return;

  auto handle = WebContents::CreateFrom(
      isolate(), managed_web_contents()->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate(), handle.ToV8());

  // Set inspected tabID.
  base::FundamentalValue tab_id(ID());
  managed_web_contents()->CallClientFunction(
      "DevToolsAPI.setInspectedTabId", &tab_id, nullptr, nullptr);

  // Inherit owner window in devtools.
  if (owner_window())
    handle->SetOwnerWindow(managed_web_contents()->GetDevToolsWebContents(),
                           owner_window());

  Emit("devtools-opened");
}

void WebContents::DevToolsClosed() {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  devtools_web_contents_.Reset();

  Emit("devtools-closed");
}

bool WebContents::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebContents, message)
    IPC_MESSAGE_HANDLER(AtomViewHostMsg_Message, OnRendererMessage)
    IPC_MESSAGE_HANDLER_DELAY_REPLY(AtomViewHostMsg_Message_Sync,
                                    OnRendererMessageSync)
    IPC_MESSAGE_HANDLER_CODE(ViewHostMsg_SetCursor, OnCursorChange,
      handled = false)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void WebContents::DestroyWebContents() {
  if (guest_delegate_) {
    guest_delegate_->Destroy();
  } else {
    delete web_contents();
  }
}

// There are three ways of destroying a webContents:
// 1. call webContents.destroy();
// 2. garbage collection;
// 3. user closes the window of webContents;
// For webview only #1 will happen, for BrowserWindow both #1 and #3 may
// happen. The #2 should never happen for webContents, because webview is
// managed by GuestViewManager, and BrowserWindow's webContents is managed
// by api::Window.
// For #1, the destructor will do the cleanup work and we only need to make
// sure "destroyed" event is emitted. For #3, the content::WebContents will
// be destroyed on close, and WebContentsDestroyed would be called for it, so
// we need to make sure the api::WebContents is also deleted.
void WebContents::WebContentsDestroyed() {
  if (is_being_destroyed_)
    return;

  is_being_destroyed_ = true;

  CommonWebContentsDelegate::DestroyWebContents();

  // clear out fullscreen state
  if (CommonWebContentsDelegate::IsFullscreenForTabOrPending(web_contents())) {
    ExitFullscreenModeForTab(web_contents());
  }

  memory_pressure_listener_.reset();

  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  Emit("will-destroy");

  // Cleanup relationships with other parts.
  RemoveFromWeakMap();

  // We can not call Destroy here because we need to call Emit first, but we
  // also do not want any method to be used, so just mark as destroyed here.
  MarkDestroyed();

  Emit("destroyed");

  // Destroy the native class in next tick.
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, GetDestroyClosure());
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-commited", details.entry->GetURL(),
       details.is_in_page, details.did_replace_entry);
}

int WebContents::GetID() const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  return extensions::TabHelper::IdForTab(web_contents());
#else
  return web_contents()->GetRenderProcessHost()->GetID();
#endif
}

WebContents::Type WebContents::GetType() const {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  if (Extension::IsBackgroundPage(this))
    return BACKGROUND_PAGE;
#endif
  return type_;
}

int WebContents::GetGuestInstanceId() const {
  brave::TabViewGuest* guest =
        brave::TabViewGuest::FromWebContents(web_contents());
  if (guest) {
    return guest->guest_instance_id();
  } else {
    return -1;
  }
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return ID() == web_contents->ID();
}

void WebContents::Reload(bool ignore_cache) {
  web_contents()->UserGestureDone();
  if (ignore_cache)
    web_contents()->GetController().ReloadBypassingCache(true);
  else
    web_contents()->GetController().Reload(true);
}

void WebContents::ResumeLoadingCreatedWebContents() {
  web_contents()->ResumeLoadingCreatedWebContents();
}

void WebContents::LoadURL(const GURL& url, const mate::Dictionary& options) {
  if (!url.is_valid()) {
    Emit("did-fail-load",
         static_cast<int>(net::ERR_INVALID_URL),
         net::ErrorToShortString(net::ERR_INVALID_URL),
         url.possibly_invalid_spec(),
         true);
    return;
  }

  content::NavigationController::LoadURLParams params(url);

  GURL http_referrer;
  if (options.Get("httpReferrer", &http_referrer))
    params.referrer = content::Referrer(http_referrer.GetAsReferrer(),
                                        blink::WebReferrerPolicyDefault);

  std::string user_agent;
  if (options.Get("userAgent", &user_agent)) {
    web_contents()->SetUserAgentOverride(user_agent);
    params.override_user_agent =
        content::NavigationController::UA_OVERRIDE_TRUE;
  }

  std::string extra_headers;
  if (options.Get("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  web_contents()->UserGestureDone();
  params.transition_type = ui::PAGE_TRANSITION_TYPED;
  web_contents()->GetController().LoadURLWithParams(params);
}

void WebContents::DownloadURL(const GURL& url) {
  auto browser_context = web_contents()->GetBrowserContext();
  auto download_manager =
    content::BrowserContext::GetDownloadManager(browser_context);

  download_manager->DownloadUrl(
      content::DownloadUrlParameters::CreateForWebContentsMainFrame(
          web_contents(), url));
}

GURL WebContents::GetURL() const {
  return web_contents()->GetURL();
}

base::string16 WebContents::GetTitle() const {
  return web_contents()->GetTitle();
}

bool WebContents::IsInitialBlankNavigation() const {
  return web_contents()->GetController().IsInitialBlankNavigation();
}

bool WebContents::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContents::IsLoadingMainFrame() const {
  // Comparing site instances works because Electron always creates a new site
  // instance when navigating, regardless of origin. See AtomBrowserClient.
  return (web_contents()->GetLastCommittedURL().is_empty() ||
          web_contents()->GetSiteInstance() !=
          web_contents()->GetPendingSiteInstance()) && IsLoading();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
}

void WebContents::GoBack() {
  if (web_contents()->GetController().CanGoBack()) {
    web_contents()->GetController().GoBack();
  }
}

void WebContents::GoForward() {
  if (web_contents()->GetController().CanGoForward()) {
    web_contents()->GetController().GoForward();
  }
}

void WebContents::GoToOffset(int offset) {
  if (web_contents()->GetController().CanGoToOffset(offset)) {
    web_contents()->GetController().GoToOffset(offset);
  }
}

void WebContents::GoToIndex(int index) {
  web_contents()->GetController().GoToIndex(index);
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents()->GetController().CanGoToOffset(offset);
}

bool WebContents::CanGoBack() const {
  return web_contents()->GetController().CanGoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents()->GetController().CanGoForward();
}

int WebContents::GetCurrentEntryIndex() const {
  return web_contents()->GetController().GetCurrentEntryIndex();
}

int WebContents::GetLastCommittedEntryIndex() const {
  return web_contents()->GetController().GetLastCommittedEntryIndex();
}

int WebContents::GetEntryCount() const {
  return web_contents()->GetController().GetEntryCount();
}

const GURL& WebContents::GetURLAtIndex(int index) const {
  auto entry = web_contents()->GetController().GetEntryAtIndex(index);
  if (entry)
    return entry->GetURL();
  else
    return GURL::EmptyGURL();
}

const base::string16 WebContents::GetTitleAtIndex(int index) const {
  auto entry = web_contents()->GetController().GetEntryAtIndex(index);
  if (entry)
    return entry->GetTitle();
  else
    return base::string16();
}

// TODO(bridiver) there should be a more generic way
// to set renderer preferences in general
const std::string& WebContents::GetWebRTCIPHandlingPolicy() const {
  return web_contents()->
    GetMutableRendererPrefs()->webrtc_ip_handling_policy;
}

void WebContents::SetWebRTCIPHandlingPolicy(
    const std::string webrtc_ip_handling_policy) {
  if (GetWebRTCIPHandlingPolicy() == webrtc_ip_handling_policy)
    return;

  // valid values from privacy.json are: "default",
  // "default_public_and_private_interfaces",
  // "default_public_interface_only", "disable_non_proxied_udp"
  web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy =
    webrtc_ip_handling_policy;

  content::RenderViewHost* host = web_contents()->GetRenderViewHost();
  if (host)
    host->SyncRendererPrefs();
}

void WebContents::ShowRepostFormWarningDialog(content::WebContents* source) {
  if (Emit("repost-form-warning"))
    source->GetController().CancelPendingReload();
  else
    source->GetController().ContinuePendingReload();
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::SetUserAgent(const std::string& user_agent,
                               mate::Arguments* args) {
  web_contents()->SetUserAgentOverride(user_agent);
}

std::string WebContents::GetUserAgent() {
  return web_contents()->GetUserAgentOverride();
}

bool WebContents::SavePage(const base::FilePath& full_file_path,
                           const content::SavePageType& save_type,
                           const SavePageHandler::SavePageCallback& callback) {
  auto handler = new SavePageHandler(web_contents(), callback);
  return handler->Handle(full_file_path, save_type);
}

void WebContents::OpenDevTools(mate::Arguments* args) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  std::string state;
  if (type_ == WEB_VIEW || !owner_window()) {
    state = "detach";
  } else if (args && args->Length() == 1) {
    bool detach = false;
    mate::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("mode", &state);

      // TODO(kevinsawicki) Remove in 2.0
      options.Get("detach", &detach);
      if (state.empty() && detach)
        state = "detach";
    }
  }
  managed_web_contents()->SetDockState(state);
  managed_web_contents()->ShowDevTools();
}

void WebContents::CloseDevTools() {
  if (type_ == REMOTE)
    return;

  managed_web_contents()->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->IsDevToolsViewShowing();
}

bool WebContents::IsDevToolsFocused() {
  if (type_ == REMOTE)
    return false;

  return managed_web_contents()->GetView()->IsDevToolsViewFocused();
}

void WebContents::EnableDeviceEmulation(
    const blink::WebDeviceEmulationParams& params) {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_EnableDeviceEmulation(routing_id(), params));
}

void WebContents::DisableDeviceEmulation() {
  if (type_ == REMOTE)
    return;

  Send(new ViewMsg_DisableDeviceEmulation(routing_id()));
}

void WebContents::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContents::InspectElement(int x, int y) {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  if (!managed_web_contents()->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  scoped_refptr<content::DevToolsAgentHost> agent(
    content::DevToolsAgentHost::GetOrCreateFor(web_contents()));
  agent->InspectElement(static_cast<brightray::InspectableWebContentsImpl*>(
      managed_web_contents()), x, y);
}

void WebContents::InspectServiceWorker() {
  if (type_ == REMOTE)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeServiceWorker) {
      OpenDevTools(nullptr);
      managed_web_contents()->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::HasServiceWorker(
    const base::Callback<void(bool)>& callback) {
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->CheckHasServiceWorker(web_contents()->GetLastCommittedURL(),
                                 GURL::EmptyGURL(),
                                 callback);
}

void WebContents::UnregisterServiceWorker(
    const base::Callback<void(bool)>& callback) {
  auto context = GetServiceWorkerContext(web_contents());
  if (!context)
    return;

  context->UnregisterServiceWorker(web_contents()->GetLastCommittedURL(),
                                   callback);
}

void WebContents::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContents::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

void WebContents::Print(mate::Arguments* args) {
  printing::PrintSettings settings;
  if (args->Length() == 1 && !args->GetNext(&settings)) {
    args->ThrowError();
    return;
  }

  printing::PrintViewManagerBasic::FromWebContents(web_contents())->
       PrintNow();
}

void WebContents::PrintToPDF(const base::DictionaryValue& setting,
                             const PrintToPDFCallback& callback) {
  printing::PrintPreviewMessageHandler::FromWebContents(web_contents())->
       PrintToPDF(setting, callback);
}

int WebContents::GetContentWindowId() {
  if (guest_delegate_)
    return guest_delegate_->proxy_routing_id();
  else
    return MSG_ROUTING_NONE;
}

void WebContents::AddWorkSpace(mate::Arguments* args,
                               const base::FilePath& path) {
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
  DevToolsAddFileSystem(path);
}

void WebContents::RemoveWorkSpace(mate::Arguments* args,
                                  const base::FilePath& path) {
  if (path.empty()) {
    args->ThrowError("path cannot be empty");
    return;
  }
  DevToolsRemoveFileSystem(path);
}

void WebContents::Undo() {
  web_contents()->Undo();
}

void WebContents::Redo() {
  web_contents()->Redo();
}

void WebContents::Cut() {
  web_contents()->Cut();
}

void WebContents::Copy() {
  web_contents()->Copy();
}

void WebContents::Paste() {
  web_contents()->Paste();
}

void WebContents::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
}

void WebContents::Delete() {
  web_contents()->Delete();
}

void WebContents::SelectAll() {
  web_contents()->SelectAll();
}

void WebContents::Unselect() {
  web_contents()->Unselect();
}

void WebContents::Replace(const base::string16& word) {
  web_contents()->Replace(word);
}

void WebContents::ReplaceMisspelling(const base::string16& word) {
  web_contents()->ReplaceMisspelling(word);
}

uint32_t WebContents::FindInPage(mate::Arguments* args) {
  uint32_t request_id = GetNextRequestId();
  base::string16 search_text;
  blink::WebFindOptions options;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    args->ThrowError("Must provide a non-empty search content");
    return 0;
  }

  args->GetNext(&options);

  web_contents()->Find(request_id, search_text, options);
  return request_id;
}

void WebContents::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void WebContents::ShowDefinitionForSelection() {
#if defined(OS_MACOSX)
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContents::CopyImageAt(int x, int y) {
  const auto host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void WebContents::Focus() {
  web_contents()->Focus();
}

#if !defined(OS_MACOSX)
bool WebContents::IsFocused() const {
  auto view = web_contents()->GetRenderWidgetHostView();
  if (!view) return false;

  if (GetType() != BACKGROUND_PAGE) {
    auto window = web_contents()->GetNativeView()->GetToplevelWindow();
    if (window && !window->IsVisible())
      return false;
  }

  return view->HasFocus();
}
#endif

void WebContents::OnCloneCreated(const mate::Dictionary& options,
    base::Callback<void(content::WebContents*)> callback,
    content::WebContents* clone) {
  brave::TabViewGuest* guest = brave::TabViewGuest::FromWebContents(clone);
  if (guest_delegate_) {
    guest->SetOpener(static_cast<brave::TabViewGuest*>(guest_delegate_));
  }
  callback.Run(clone);
}

void WebContents::Clone(mate::Arguments* args) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());

  mate::Dictionary options;

  if (!args->PeekNext()->IsFunction()) {
    if (!args->GetNext(&options)) {
      args->ThrowError("Invalid argument `options`");
      return;
    }
  } else {
    options = mate::Dictionary::CreateEmpty(isolate());
  }

  base::Callback<void(content::WebContents*)> callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError("`callback` is a required field");
    return;
  }

  if (!IsGuest()) {
    callback.Run(nullptr);
    return;
  }

  base::DictionaryValue create_params;
  create_params.SetString("partition",
      static_cast<brave::BraveBrowserContext*>(
          GetBrowserContext())->partition_with_prefix());

  create_params.SetBoolean("clone", true);

  auto guest_view_manager =
      static_cast<GuestViewManager*>(GetBrowserContext()->GetGuestManager());

  if (!guest_view_manager) {
    callback.Run(nullptr);
    return;
  }

  options.Set("userGesture", true);

  guest_view_manager->CreateGuest(brave::TabViewGuest::Type,
      HostWebContents(),
      create_params,
      base::Bind(&WebContents::OnCloneCreated, base::Unretained(this), options,
        base::Bind(&WebContents::OnTabCreated, base::Unretained(this),
            options, callback)));
}

void WebContents::SetActive(bool active) {
  if (active)
    web_contents()->WasShown();

#if BUILDFLAG(ENABLE_EXTENSIONS)
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (tab_helper)
    tab_helper->SetActive(active);
#endif

  Emit("set-active", active);
}

void WebContents::SetTabIndex(int index) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (tab_helper)
    tab_helper->SetTabIndex(index);
#endif

  Emit("set-tab-index", index);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
bool WebContents::ExecuteScriptInTab(mate::Arguments* args) {
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (!tab_helper)
    return false;

  return tab_helper->ExecuteScriptInTab(args);
}

void WebContents::SetTabValues(const base::DictionaryValue& values) {
  auto tab_helper = extensions::TabHelper::FromWebContents(web_contents());
  if (!tab_helper)
    return;

  return tab_helper->SetTabValues(values);
}
#endif

bool WebContents::SendIPCMessage(bool all_frames,
                                 const base::string16& channel,
                                 const base::ListValue& args) {
  return Send(new AtomViewMsg_Message(routing_id(), all_frames, channel, args));
}

void WebContents::SendInputEvent(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input_event) {
  const auto view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;
  const auto host = view->GetRenderWidgetHost();
  if (!host)
    return;

  int type = mate::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::isMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_event)) {
      host->ForwardMouseEvent(mouse_event);
      return;
    }
  } else if (blink::WebInputEvent::isKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event;
    if (mate::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      host->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::MouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (mate::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      host->ForwardWheelEvent(mouse_wheel_event);
      return;
    }
  }

  isolate->ThrowException(v8::Exception::Error(mate::StringToV8(
      isolate, "Invalid event object")));
}

void WebContents::StartDrag(const mate::Dictionary& item,
                            mate::Arguments* args) {
  base::FilePath file;
  std::vector<base::FilePath> files;
  if (!item.Get("files", &files) && item.Get("file", &file)) {
    files.push_back(file);
  }

  mate::Handle<NativeImage> icon;
  if (!item.Get("icon", &icon) && !file.empty()) {
    // TODO(zcbenz): Set default icon from file.
  }

  // Error checking.
  if (icon.IsEmpty()) {
    args->ThrowError("icon must be set");
    return;
  }

  // Start dragging.
  if (!files.empty()) {
    base::MessageLoop::ScopedNestableTaskAllower allow(
        base::MessageLoop::current());
    DragFileItems(files, icon->image(), web_contents()->GetNativeView());
  } else {
    args->ThrowError("There is nothing to drag");
  }
}

void WebContents::CapturePage(mate::Arguments* args) {
  gfx::Rect rect;
  base::Callback<void(const gfx::Image&)> callback;

  if (!(args->Length() == 1 && args->GetNext(&callback)) &&
      !(args->Length() == 2 && args->GetNext(&rect)
                            && args->GetNext(&callback))) {
    args->ThrowError();
    return;
  }

  const auto view = web_contents()->GetRenderWidgetHostView();
  const auto host = view ? view->GetRenderWidgetHost() : nullptr;
  if (!view || !host) {
    callback.Run(gfx::Image());
    return;
  }

  // Capture full page if user doesn't specify a |rect|.
  const gfx::Size view_size = rect.IsEmpty() ? view->GetViewBounds().size() :
                                               rect.size();

  // By default, the requested bitmap size is the view size in screen
  // coordinates.  However, if there's more pixel detail available on the
  // current system, increase the requested bitmap size to capture it all.
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  const float scale =
      display::Screen::GetScreen()->GetDisplayNearestWindow(native_view)
      .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  host->CopyFromBackingStore(gfx::Rect(rect.origin(), view_size),
                             bitmap_size,
                             base::Bind(&OnCapturePageDone, callback),
                             kBGRA_8888_SkColorType);
}

gfx::Size WebContents::GetPreferredSize() {
  return web_contents()->GetPreferredSize();
}

void WebContents::OnCursorChange(const content::WebCursor& cursor) {
  content::WebCursor::CursorInfo info;
  cursor.GetCursorInfo(&info);

  if (cursor.IsCustom()) {
    Emit("cursor-changed", CursorTypeToString(info),
      gfx::Image::CreateFrom1xBitmap(info.custom_image),
      info.image_scale_factor,
      gfx::Size(info.custom_image.width(), info.custom_image.height()),
      info.hotspot);
  } else {
    Emit("cursor-changed", CursorTypeToString(info));
  }
}

void WebContents::SetSize(const SetSizeParams& params) {
  NOTIMPLEMENTED();
  // if (guest_delegate_)
  //   guest_delegate_->SetSize(params);
}

bool WebContents::IsGuest() const {
  return type_ == WEB_VIEW;
}

v8::Local<v8::Value> WebContents::GetWebPreferences(v8::Isolate* isolate) {
  WebContentsPreferences* web_preferences =
      WebContentsPreferences::FromWebContents(web_contents());
  return mate::ConvertToV8(isolate, *web_preferences->web_preferences());
}

v8::Local<v8::Value> WebContents::GetOwnerBrowserWindow() {
  if (owner_window())
    return Window::From(isolate(), owner_window());
  else
    return v8::Null(isolate());
}

void WebContents::SetZoomLevel(double zoom) {
  zoom::ZoomController* zoom_controller =
      zoom::ZoomController::FromWebContents(web_contents());
  if (!zoom_controller)
    return;

  zoom_controller->SetZoomLevel(zoom);
}

void WebContents::ZoomIn() {
  zoom::PageZoom::Zoom(web_contents(),
                          content::PAGE_ZOOM_IN);
}

void WebContents::ZoomOut() {
  zoom::PageZoom::Zoom(web_contents(),
                          content::PAGE_ZOOM_OUT);
}

void WebContents::ZoomReset() {
  zoom::PageZoom::Zoom(web_contents(),
                          content::PAGE_ZOOM_RESET);
}

int WebContents::GetZoomPercent() {
  zoom::ZoomController* zoom_controller =
      zoom::ZoomController::FromWebContents(web_contents());
  if (!zoom_controller)
    return 100;

  return zoom_controller->GetZoomPercent();
}

void WebContents::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel memory_pressure_level) {
  if (atom::Browser::Get()->is_shutting_down())
    return;

  if (memory_pressure_level ==
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL) {
    web_contents()->GetController().ClearAllScreenshots();
  }

  if (web_contents()->GetRenderProcessHost() &&
      !web_contents()->GetRenderWidgetHostView()) {
    content::MemoryPressureController::SendPressureNotification(
      web_contents()->GetRenderProcessHost(), memory_pressure_level);
    return;
  }

  // TODO(bridiver) only run once per render process
  if (web_contents()->GetRenderWidgetHostView() &&
      !web_contents()->GetRenderWidgetHostView()->HasFocus()) {
    content::MemoryPressureController::SendPressureNotification(
      web_contents()->GetRenderProcessHost(), memory_pressure_level);
  }
}

int32_t WebContents::ID() const {
  return weak_map_id();
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  if (session_.IsEmpty()) {
    auto context =
        static_cast<AtomBrowserContext*>(web_contents()->GetBrowserContext());
    auto session = Session::CreateFrom(isolate, context);
    session_.Reset(isolate, session.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, session_);
}

content::WebContents* WebContents::HostWebContents() {
  if (guest_delegate_)
    return guest_delegate_->embedder_web_contents();
  return nullptr;
}

v8::Local<v8::Value> WebContents::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> WebContents::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = atom::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

// static
void WebContents::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebContents"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .MakeDestroyable()
      .SetMethod("getId", &WebContents::GetID)
      .SetMethod("equal", &WebContents::Equal)
      .SetMethod("_loadURL", &WebContents::LoadURL)
      .SetMethod("_reload", &WebContents::Reload)
      .SetMethod("downloadURL", &WebContents::DownloadURL)
      .SetMethod("getURL", &WebContents::GetURL)
      .SetMethod("getTitle", &WebContents::GetTitle)
      .SetMethod("isInitialBlankNavigation",
          &WebContents::IsInitialBlankNavigation)
      .SetMethod("isLoading", &WebContents::IsLoading)
      .SetMethod("isLoadingMainFrame", &WebContents::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
      .SetMethod("stop", &WebContents::Stop)
      .SetMethod("goBack", &WebContents::GoBack)
      .SetMethod("goForward", &WebContents::GoForward)
      .SetMethod("goToOffset", &WebContents::GoToOffset)
      .SetMethod("goToIndex", &WebContents::GoToIndex)
      .SetMethod("canGoBack", &WebContents::CanGoBack)
      .SetMethod("canGoForward", &WebContents::CanGoForward)
      .SetMethod("canGoToOffset", &WebContents::CanGoToOffset)
      .SetMethod("getURLAtIndex", &WebContents::GetURLAtIndex)
      .SetMethod("getTitleAtIndex", &WebContents::GetTitleAtIndex)
      .SetMethod("getEntryCount", &WebContents::GetEntryCount)
      .SetMethod("getCurrentEntryIndex", &WebContents::GetCurrentEntryIndex)
      .SetMethod("getLastCommittedEntryIndex",
                 &WebContents::GetLastCommittedEntryIndex)
      .SetMethod("isCrashed", &WebContents::IsCrashed)
      .SetMethod("setUserAgent", &WebContents::SetUserAgent)
      .SetMethod("getUserAgent", &WebContents::GetUserAgent)
      .SetMethod("savePage", &WebContents::SavePage)
      .SetMethod("openDevTools", &WebContents::OpenDevTools)
      .SetMethod("closeDevTools", &WebContents::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContents::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation",
                 &WebContents::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation",
                 &WebContents::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &WebContents::ToggleDevTools)
      .SetMethod("inspectElement", &WebContents::InspectElement)
      .SetMethod("setAudioMuted", &WebContents::SetAudioMuted)
      .SetMethod("isAudioMuted", &WebContents::IsAudioMuted)
      .SetMethod("undo", &WebContents::Undo)
      .SetMethod("redo", &WebContents::Redo)
      .SetMethod("cut", &WebContents::Cut)
      .SetMethod("copy", &WebContents::Copy)
      .SetMethod("paste", &WebContents::Paste)
      .SetMethod("pasteAndMatchStyle", &WebContents::PasteAndMatchStyle)
      .SetMethod("delete", &WebContents::Delete)
      .SetMethod("selectAll", &WebContents::SelectAll)
      .SetMethod("unselect", &WebContents::Unselect)
      .SetMethod("replace", &WebContents::Replace)
      .SetMethod("replaceMisspelling", &WebContents::ReplaceMisspelling)
      .SetMethod("findInPage", &WebContents::FindInPage)
      .SetMethod("stopFindInPage", &WebContents::StopFindInPage)
      .SetMethod("focus", &WebContents::Focus)
      .SetMethod("isFocused", &WebContents::IsFocused)
      .SetMethod("_clone", &WebContents::Clone)
      .SetMethod("_send", &WebContents::SendIPCMessage)
      .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
      .SetMethod("startDrag", &WebContents::StartDrag)
      .SetMethod("setSize", &WebContents::SetSize)
      .SetMethod("isGuest", &WebContents::IsGuest)
      .SetMethod("getType", &WebContents::GetType)
      .SetMethod("getWebPreferences", &WebContents::GetWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &WebContents::GetOwnerBrowserWindow)
      .SetMethod("hasServiceWorker", &WebContents::HasServiceWorker)
      .SetMethod("unregisterServiceWorker",
                 &WebContents::UnregisterServiceWorker)
      .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
      .SetMethod("print", &WebContents::Print)
      .SetMethod("_printToPDF", &WebContents::PrintToPDF)
      .SetMethod("addWorkSpace", &WebContents::AddWorkSpace)
      .SetMethod("removeWorkSpace", &WebContents::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &WebContents::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &WebContents::CopyImageAt)
      .SetMethod("capturePage", &WebContents::CapturePage)
      .SetMethod("getPreferredSize", &WebContents::GetPreferredSize)
      .SetProperty("id", &WebContents::ID)
      .SetMethod("getContentWindowId", &WebContents::GetContentWindowId)
      .SetMethod("setActive", &WebContents::SetActive)
      .SetMethod("setTabIndex", &WebContents::SetTabIndex)
      .SetMethod("setWebRTCIPHandlingPolicy",
                  &WebContents::SetWebRTCIPHandlingPolicy)
      .SetMethod("getWebRTCIPHandlingPolicy",
                  &WebContents::GetWebRTCIPHandlingPolicy)
      .SetMethod("setZoomLevel",
                  &WebContents::SetZoomLevel)
      .SetMethod("zoomIn",
                  &WebContents::ZoomIn)
      .SetMethod("zoomOut",
                  &WebContents::ZoomOut)
      .SetMethod("zoomReset",
                  &WebContents::ZoomReset)
      .SetMethod("getZoomPercent",
                  &WebContents::GetZoomPercent)
      .SetMethod("enablePreferredSizeMode",
                  &WebContents::EnablePreferredSizeMode)
      .SetMethod("authorizePlugin",
                  &WebContents::AuthorizePlugin)
#if BUILDFLAG(ENABLE_EXTENSIONS)
      .SetMethod("executeScriptInTab", &WebContents::ExecuteScriptInTab)
      .SetMethod("setTabValues", &WebContents::SetTabValues)
#endif
      .SetMethod("close", &WebContents::CloseContents)
      .SetMethod("autofillSelect", &WebContents::AutofillSelect)
      .SetMethod("autofillPopupHidden", &WebContents::AutofillPopupHidden)
      .SetProperty("session", &WebContents::Session)
      .SetProperty("guestInstanceId", &WebContents::GetGuestInstanceId)
      .SetProperty("hostWebContents", &WebContents::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContents::DevToolsWebContents)
      .SetProperty("debugger", &WebContents::Debugger);
}

AtomBrowserContext* WebContents::GetBrowserContext() const {
  return static_cast<AtomBrowserContext*>(web_contents()->GetBrowserContext());
}

void WebContents::OnRendererMessage(const base::string16& channel,
                                    const base::ListValue& args) {
  // webContents.emit(channel, new Event(), args...);
  Emit(base::UTF16ToUTF8(channel), args);
}

void WebContents::OnRendererMessageSync(const base::string16& channel,
                                        const base::ListValue& args,
                                        IPC::Message* message) {
  // webContents.emit(channel, new Event(sender, message), args...);
  EmitWithSender(base::UTF16ToUTF8(channel), web_contents(), message, args);
}

// static
mate::Handle<WebContents> WebContents::FromTabID(v8::Isolate* isolate,
    int tab_id) {
  return CreateFrom(isolate,
      extensions::TabHelper::GetTabById(tab_id));
}

void WebContents::OnTabCreated(const mate::Dictionary& options,
    base::Callback<void(content::WebContents*)> callback,
    content::WebContents* tab) {
  node::Environment* env = node::Environment::GetCurrent(isolate());
  if (!env) {
    return;
  }

  auto event = v8::Local<v8::Object>::Cast(
      mate::Event::Create(isolate()).ToV8());

  mate::EmitEvent(isolate(),
                  env->process_object(),
                  "on-tab-created",
                  tab,
                  options);

  bool active = true;
  options.Get("active", &active);

  bool user_gesture = false;
  options.Get("userGesture", &user_gesture);

  bool was_blocked = false;
  AddNewContents(web_contents(),
                    tab,
                    active ?
                      WindowOpenDisposition::NEW_FOREGROUND_TAB :
                      WindowOpenDisposition::NEW_BACKGROUND_TAB,
                    gfx::Rect(),
                    user_gesture,
                    &was_blocked);

  if (was_blocked)
    callback.Run(nullptr);
  else
    callback.Run(tab);
}

// static
void WebContents::CreateTab(mate::Arguments* args) {
  if (args->Length() != 4) {
    args->ThrowError("Wrong number of arguments");
    return;
  }

  WebContents* owner;
  if (!args->GetNext(&owner)) {
    args->ThrowError("`owner` is a required field");
    return;
  }

  mate::Handle<api::Session> session;
  if (!args->GetNext(&session)) {
    args->ThrowError("`session` is a required field");
    return;
  }

  mate::Dictionary options;
  if (!args->GetNext(&options)) {
    args->ThrowError("`options` is a required field");
    return;
  }

  base::Callback<void(content::WebContents*)> callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError("`callback` is a required field");
    return;
  }

  auto browser_context = session->browser_context();

  auto guest_view_manager =
      static_cast<GuestViewManager*>(browser_context->GetGuestManager());

  if (!guest_view_manager) {
    args->ThrowError("No guest view manager");
    return;
  }

  base::DictionaryValue create_params;
  std::string src;
  if (options.Get("src", &src) || options.Get("url", &src)) {
    create_params.SetString("src", src);
  }
  create_params.SetString("partition",
      static_cast<brave::BraveBrowserContext*>(
            browser_context)->partition_with_prefix());

  guest_view_manager->CreateGuest(brave::TabViewGuest::Type,
      owner->web_contents(),
      create_params,
      base::Bind(&WebContents::OnTabCreated, base::Unretained(owner),
          options, callback));
}

// static
mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents) {
  if (!web_contents) {
    return mate::Handle<WebContents>();
  }
  // We have an existing WebContents object in JS.
  auto existing = TrackableObject::FromWrappedClass(isolate, web_contents);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<WebContents*>(existing));

  // Otherwise create a new WebContents wrapper object.
  return mate::CreateHandle(isolate, new WebContents(isolate, web_contents,
        REMOTE));
}

mate::Handle<WebContents> WebContents::CreateFrom(
    v8::Isolate* isolate, content::WebContents* web_contents, Type type) {
  if (!web_contents) {
    return mate::Handle<WebContents>();
  }

  // We have an existing WebContents object in JS.
  auto existing = TrackableObject::FromWrappedClass(isolate, web_contents);
  if (existing)
    return mate::CreateHandle(isolate, static_cast<WebContents*>(existing));

  // Otherwise create a new WebContents wrapper object.
  return mate::CreateHandle(isolate, new WebContents(isolate, web_contents,
        type));
}

// static
mate::Handle<WebContents> WebContents::Create(
    v8::Isolate* isolate, const mate::Dictionary& options) {
  return mate::CreateHandle(isolate, new WebContents(isolate, options));
}

mate::Handle<WebContents> WebContents::CreateWithParams(
    v8::Isolate* isolate, const mate::Dictionary& options,
    const content::WebContents::CreateParams& create_params) {
  return mate::CreateHandle(isolate,
      new WebContents(isolate, options, create_params));
}

}  // namespace api

}  // namespace atom

namespace {

using atom::api::WebContents;

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  mate::Dictionary dict(isolate, exports);
  dict.Set("WebContents", WebContents::GetConstructor(isolate)->GetFunction());
  dict.SetMethod("create", &WebContents::Create);
  dict.SetMethod("createTab", &WebContents::CreateTab);
  dict.SetMethod("fromTabID", &WebContents::FromTabID);
  dict.SetMethod("fromId", &mate::TrackableObject<WebContents>::FromWeakMapID);
  dict.SetMethod("getAllWebContents",
                 &mate::TrackableObject<WebContents>::GetAll);
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_web_contents, Initialize)
