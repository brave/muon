// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_
#define BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_

#include <memory>

#include "extensions/renderer/object_backed_native_handler.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

namespace brave {

class WebFrameBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit WebFrameBindings(extensions::ScriptContext* context);
  virtual ~WebFrameBindings();

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  void WebFrame(const v8::FunctionCallbackInfo<v8::Value>& args);

  void SetZoomLevel(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetZoomLevelLimits(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetPageScaleLimits(const v8::FunctionCallbackInfo<v8::Value>& args);

  void SetGlobal(const v8::FunctionCallbackInfo<v8::Value>& args);
  void ExecuteJavaScript(const v8::FunctionCallbackInfo<v8::Value>& args);

 private:
  void BindToGC(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(WebFrameBindings);
};

}  // namespace brave

#endif  // BRAVE_RENDERER_EXTENSIONS_WEB_FRAME_BINDINGS_H_
