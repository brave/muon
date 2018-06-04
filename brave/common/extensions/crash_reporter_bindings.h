// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_CRASH_REPORTER_BINDINGS_H_
#define BRAVE_COMMON_EXTENSIONS_CRASH_REPORTER_BINDINGS_H_

#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace brave {

class CrashReporterBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit CrashReporterBindings(extensions::ScriptContext* context);
  ~CrashReporterBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  static v8::Local<v8::Object> API(extensions::ScriptContext* context);
 private:
  void SetEnabled(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetChannelCrashValue(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetJavascriptInfoCrashValue(
      const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetNodeEnvCrashValue(const v8::FunctionCallbackInfo<v8::Value>& args);
  void SetVersionCrashValue(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DumpWithoutCrashing(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(CrashReporterBindings);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_CRASH_REPORTER_BINDINGS_H_
