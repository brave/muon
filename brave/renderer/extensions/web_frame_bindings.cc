// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/renderer/extensions/web_frame_bindings.h"

#include <memory>
#include <string>
#include <vector>

#include "base/strings/utf_string_conversions.h"
#include "content/renderer/browser_plugin/browser_plugin.h"
#include "content/renderer/browser_plugin/browser_plugin_manager.h"
#include "extensions/renderer/console.h"
#include "extensions/renderer/guest_view/extensions_guest_view_container.h"
#include "extensions/renderer/script_context.h"
#include "native_mate/scoped_persistent.h"
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

  void Completed(
      const blink::WebVector<v8::Local<v8::Value>>& result) override {
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ScriptExecutionCallback);
};

}  // namespace

WebFrameBindings::WebFrameBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {}

WebFrameBindings::~WebFrameBindings() {}

void WebFrameBindings::AddRoutes() {
  RouteHandlerFunction(
      "executeJavaScript",
      base::Bind(&WebFrameBindings::ExecuteJavaScript, base::Unretained(this)));
  RouteHandlerFunction(
      "setZoomLevel",
      base::Bind(&WebFrameBindings::SetZoomLevel, base::Unretained(this)));
  RouteHandlerFunction("setZoomLevelLimits",
                       base::Bind(&WebFrameBindings::SetZoomLevelLimits,
                                  base::Unretained(this)));
  RouteHandlerFunction("setPageScaleLimits",
                       base::Bind(&WebFrameBindings::SetPageScaleLimits,
                                  base::Unretained(this)));
  RouteHandlerFunction("setGlobal", base::Bind(&WebFrameBindings::SetGlobal,
                                               base::Unretained(this)));
}

void WebFrameBindings::ExecuteJavaScript(
      const v8::FunctionCallbackInfo<v8::Value>& args) {
  const base::string16 code = base::UTF8ToUTF16(mate::V8ToString(args[0]));
  v8::Local<v8::Context> main_context =
      context()->web_frame()->MainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  std::unique_ptr<blink::WebScriptExecutionCallback> callback(
      new ScriptExecutionCallback());
  context()->web_frame()->RequestExecuteScriptAndReturnValue(
      blink::WebScriptSource(blink::WebString::FromUTF16(code)),
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

  if (value->IsNull() || value->IsUndefined()) {
    extensions::console::AddMessage(
        context(), content::CONSOLE_MESSAGE_LEVEL_WARNING,
        "cannot set path on null or undefined value");
    return;
  }

  v8::Local<v8::Context> main_context =
      context()->web_frame()->MainWorldScriptContext();
  v8::Context::Scope context_scope(main_context);

  if (!ContextCanAccessObject(main_context, main_context->Global(), false)) {
    extensions::console::AddMessage(
        context(), content::CONSOLE_MESSAGE_LEVEL_WARNING,
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
    } else if (iter == path.end() - 1) {
      obj->Set(*iter, value);
    } else {
      obj = v8::Handle<v8::Object>::Cast(obj->Get(*iter));
    }
    if ((obj->IsNull() || obj->IsUndefined()) && iter != path.end() - 1) {
      extensions::console::AddMessage(
          context(), content::CONSOLE_MESSAGE_LEVEL_WARNING,
          "invalid path for setGlobal");
      return;
    }
  }
}

void WebFrameBindings::SetZoomLevel(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 1);
  CHECK(args[0]->IsInt32());
  double level = args[0].As<v8::Number>()->Value();
  context()->web_frame()->View()->SetZoomLevel(level);
}

void WebFrameBindings::SetZoomLevelLimits(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 2);
  CHECK(args[0]->IsInt32());
  CHECK(args[1]->IsInt32());
  float min_level = args[0].As<v8::Number>()->Value();
  float max_level = args[1].As<v8::Number>()->Value();
  context()->web_frame()->View()->ZoomLimitsChanged(min_level, max_level);
}

void WebFrameBindings::SetPageScaleLimits(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  CHECK_EQ(args.Length(), 2);
  CHECK(args[0]->IsInt32());
  CHECK(args[1]->IsInt32());
  auto web_frame = context()->web_frame();
  float min_scale = args[0].As<v8::Number>()->Value();
  float max_scale = args[1].As<v8::Number>()->Value();
  web_frame->View()->SetDefaultPageScaleLimits(min_scale, max_scale);
  web_frame->View()->SetIgnoreViewportTagScaleLimits(true);
}

}  // namespace brave
