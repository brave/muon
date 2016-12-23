// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/renderer/extensions/web_frame_bindings.h"

#include <memory>
#include <string>
#include <vector>

#include "atom/renderer/api/atom_api_spell_check_client.h"
#include "base/strings/utf_string_conversions.h"
#include "content/renderer/browser_plugin/browser_plugin.h"
#include "content/renderer/browser_plugin/browser_plugin_manager.h"
#include "extensions/renderer/console.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "extensions/renderer/script_context.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebScriptExecutionCallback.h"
#include "third_party/WebKit/public/web/WebScriptSource.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace brave {

namespace {

class ScriptExecutionCallback : public blink::WebScriptExecutionCallback {
 public:
  ScriptExecutionCallback() {}
  ~ScriptExecutionCallback() override {}

  void completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScriptExecutionCallback);
};

}  // namespace

WebFrameBindings::WebFrameBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction(
      "executeJavaScript",
      base::Bind(&WebFrameBindings::ExecuteJavaScript,
          base::Unretained(this)));
  RouteFunction(
      "setSpellCheckProvider",
      base::Bind(&WebFrameBindings::SetSpellCheckProvider,
          base::Unretained(this)));
  RouteFunction(
      "setZoomLevel",
      base::Bind(&WebFrameBindings::SetZoomLevel,
          base::Unretained(this)));
  RouteFunction(
      "setZoomLevelLimits",
      base::Bind(&WebFrameBindings::SetZoomLevelLimits,
          base::Unretained(this)));
  RouteFunction(
      "setPageScaleLimits",
      base::Bind(&WebFrameBindings::SetPageScaleLimits,
          base::Unretained(this)));
  RouteFunction(
      "setGlobal",
      base::Bind(&WebFrameBindings::SetGlobal, base::Unretained(this)));
  RouteFunction(
      "registerElementResizeCallback",
      base::Bind(&WebFrameBindings::RegisterElementResizeCallback,
          base::Unretained(this)));
  RouteFunction(
      "registerEmbedderCustomElement",
      base::Bind(&WebFrameBindings::RegisterEmbedderCustomElement,
          base::Unretained(this)));
}

WebFrameBindings::~WebFrameBindings() {
}

void WebFrameBindings::Invalidate() {
  // only remove the spell check client when the main frame is invalidated
  if (!context()->web_frame()->parent()) {
    context()->web_frame()->view()->setSpellCheckClient(nullptr);
    spell_check_client_.reset(nullptr);
  }
  ObjectBackedNativeHandler::Invalidate();
}

void WebFrameBindings::RegisterElementResizeCallback(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  // There are two parameters.
  CHECK_EQ(args.Length(), 2);
  // Element Instance ID.
  CHECK(args[0]->IsInt32());
  // Callback function.
  CHECK(args[1]->IsFunction());

  int element_instance_id = args[0]->Int32Value();
  // An element instance ID uniquely identifies a ExtensionsGuestViewContainer
  // within a RenderView.
  auto* guest_view_container =
      static_cast<extensions::ExtensionsGuestViewContainer*>(
          guest_view::GuestViewContainer::FromID(element_instance_id));
  if (!guest_view_container)
    return;

  guest_view_container->RegisterElementResizeCallback(
      args[1].As<v8::Function>(), args.GetIsolate());

  args.GetReturnValue().Set(v8::Boolean::New(context()->isolate(), true));
}

void WebFrameBindings::RegisterEmbedderCustomElement(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK(args.Length() >= 1 &&
        args[0]->IsString());

  const base::string16 name = base::UTF8ToUTF16(mate::V8ToString(args[0]));
  v8::Local<v8::Value> options =
      static_cast<v8::Local<v8::Value>>(args[1]);

  blink::WebExceptionCode c = 0;
  args.GetReturnValue().Set(
    context()->web_frame()->
        document().registerEmbedderCustomElement(name, options, c));
}

void WebFrameBindings::SetSpellCheckProvider(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (context()->web_frame()->parent()) {
    extensions::console::Warn(context()->GetRenderFrame(),
      "spellcheck provider can only be set by the main frame");
    return;
  }

  const std::string lang = mate::V8ToString(args[0].As<v8::String>());
  bool auto_spell_correct_turned_on = args[1].As<v8::Boolean>()->Value();
  v8::Local<v8::Object> provider = v8::Local<v8::Object>::Cast(args[2]);

  std::unique_ptr<atom::api::SpellCheckClient> spell_check_client(
      new atom::api::SpellCheckClient(
          lang, auto_spell_correct_turned_on, GetIsolate(), provider));
  context()->web_frame()->view()->setSpellCheckClient(spell_check_client.get());
  spell_check_client_.swap(spell_check_client);
}

void WebFrameBindings::ExecuteJavaScript(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  const base::string16 code = base::UTF8ToUTF16(mate::V8ToString(args[0]));
  v8::Local<v8::Context> main_context =
      context()->web_frame()->mainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback());
  context()->web_frame()->requestExecuteScriptAndReturnValue(
      blink::WebScriptSource(code),
      true,
      callback.release());
}

void WebFrameBindings::SetGlobal(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  v8::Isolate* isolate = GetIsolate();
  v8::Local<v8::Array> array(v8::Local<v8::Array>::Cast(args[0]));
  std::vector<v8::Handle<v8::String>> path;
  mate::Converter<std::vector<v8::Handle<v8::String>>>::FromV8(
      isolate, array, &path);
  v8::Local<v8::Object> value = v8::Local<v8::Object>::Cast(args[1]);

  v8::Local<v8::Context> main_context =
      context()->web_frame()->mainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  if (!ContextCanAccessObject(main_context, main_context->Global(), false)) {
    extensions::console::Warn(context()->GetRenderFrame(),
      "cannot access global in main frame script context");
    return;
  }

  v8::Handle<v8::Object> obj;
  for (std::vector<v8::Handle<v8::String>>::const_iterator iter =
             path.begin();
         iter != path.end();
         ++iter) {
    if (iter == path.begin()) {
      obj = v8::Handle<v8::Object>::Cast(main_context->Global()->Get(*iter));
    } else if (iter == path.end()-1) {
      obj->Set(*iter, value);
    } else {
      obj = v8::Handle<v8::Object>::Cast(obj->Get(*iter));
    }
  }
}

void WebFrameBindings::SetZoomLevel(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 1);
  CHECK(args[0]->IsInt32());
  double level = args[0].As<v8::Number>()->Value();
  context()->web_frame()->view()->setZoomLevel(level);
}

// double WebFrame::GetZoomLevel() const {
//   return web_frame_->view()->zoomLevel();
// }

// double WebFrame::SetZoomFactor(double factor) {
//   return blink::WebView::zoomLevelToZoomFactor(SetZoomLevel(
//       blink::WebView::zoomFactorToZoomLevel(factor)));
// }

// double WebFrame::GetZoomFactor() const {
//   return blink::WebView::zoomLevelToZoomFactor(GetZoomLevel());
// }

void WebFrameBindings::SetZoomLevelLimits(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 2);
  CHECK(args[0]->IsInt32());
  CHECK(args[1]->IsInt32());
  float min_level = args[0].As<v8::Number>()->Value();
  float max_level = args[1].As<v8::Number>()->Value();
  context()->web_frame()->view()->zoomLimitsChanged(min_level, max_level);
}

// float WebFrame::GetPageScaleFactor() {
//   return web_frame_->view()->pageScaleFactor();
// }

// void WebFrame::SetPageScaleFactor(float factor) {
//   web_frame_->view()->setPageScaleFactor(factor);
// }

void WebFrameBindings::SetPageScaleLimits(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 2);
  CHECK(args[0]->IsInt32());
  CHECK(args[1]->IsInt32());
  auto web_frame = context()->web_frame();
  float min_scale = args[0].As<v8::Number>()->Value();
  float max_scale = args[1].As<v8::Number>()->Value();
  web_frame->view()->setDefaultPageScaleLimits(min_scale, max_scale);
  web_frame->view()->setIgnoreViewportTagScaleLimits(true);
}

// float WebFrame::GetTextZoomFactor() {
//   return web_frame_->view()->textZoomFactor();
// }

// void WebFrame::SetTextZoomFactor(float factor) {
//   web_frame_->view()->setTextZoomFactor(factor);
// }

}  // namespace brave
