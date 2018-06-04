// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/extensions/crash_reporter_bindings.h"

#include <memory>

#include "base/debug/dump_without_crashing.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "v8/include/v8.h"

namespace brave {

CrashReporterBindings::CrashReporterBindings(
        extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {}

CrashReporterBindings::~CrashReporterBindings() {}

void CrashReporterBindings::AddRoutes() {
  RouteHandlerFunction(
      "SetEnabled",
      base::Bind(&CrashReporterBindings::SetEnabled, base::Unretained(this)));
  RouteHandlerFunction("SetChannelCrashValue",
                       base::Bind(&CrashReporterBindings::SetChannelCrashValue,
                                  base::Unretained(this)));
  RouteHandlerFunction(
      "SetJavascriptInfoCrashValue",
      base::Bind(&CrashReporterBindings::SetJavascriptInfoCrashValue,
                 base::Unretained(this)));
  RouteHandlerFunction("SetNodeEnvCrashValue",
                       base::Bind(&CrashReporterBindings::SetNodeEnvCrashValue,
                                  base::Unretained(this)));
  RouteHandlerFunction("SetVersionCrashValue",
                       base::Bind(&CrashReporterBindings::SetVersionCrashValue,
                                  base::Unretained(this)));
  RouteHandlerFunction("DumpWithoutCrashing",
                       base::Bind(&CrashReporterBindings::DumpWithoutCrashing,
                                  base::Unretained(this)));
}

// static
v8::Local<v8::Object> CrashReporterBindings::API(
    extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_crash_reporter", std::unique_ptr<extensions::NativeHandler>(
        new CrashReporterBindings(context)));

  v8::Local<v8::Object> crash_reporter = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
      crash_reporter, "setChannelCrashValue", "muon_crash_reporter",
      "SetChannelCrashValue");
  context->module_system()->SetNativeLazyField(
      crash_reporter, "setJavascriptInfoCrashValue", "muon_crash_reporter",
      "SetJavascriptInfoCrashValue");
  context->module_system()->SetNativeLazyField(
      crash_reporter, "setNodeEnvCrashValue", "muon_crash_reporter",
      "SetNodeEnvCrashValue");
  context->module_system()->SetNativeLazyField(
      crash_reporter, "setVersionCrashValue", "muon_crash_reporter",
      "SetVersionCrashValue");
  context->module_system()->SetNativeLazyField(
      crash_reporter,
      "setEnabled", "muon_crash_reporter", "SetEnabled");
  context->module_system()->SetNativeLazyField(
      crash_reporter,
      "dumpWithoutCrashing", "muon_crash_reporter", "DumpWithoutCrashing");
  return crash_reporter;
}

void CrashReporterBindings::SetEnabled(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsBoolean()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid arguments to 'setEnabled'"));
    return;
  }

  bool enabled = args[0]->BooleanValue();
  MuonCrashReporterClient::SetCrashReportingEnabled(enabled);
}

void CrashReporterBindings::DumpWithoutCrashing(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  base::debug::DumpWithoutCrashing();
}

void CrashReporterBindings::SetChannelCrashValue(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid argument to 'setChannelCrashValue'"));
    return;
  }

  std::string value(*v8::String::Utf8Value(args[0]));
  MuonCrashReporterClient::SetChannelCrashValue(value);
}

void CrashReporterBindings::SetJavascriptInfoCrashValue(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid argument to 'setJavascriptInfoCrashValue'"));
    return;
  }

  std::string value(*v8::String::Utf8Value(args[0]));
  MuonCrashReporterClient::SetJavascriptInfoCrashValue(value);
}

void CrashReporterBindings::SetNodeEnvCrashValue(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid argument to 'setNodeEnvCrashValue'"));
    return;
  }

  std::string value(*v8::String::Utf8Value(args[0]));
  MuonCrashReporterClient::SetNodeEnvCrashValue(value);
}

void CrashReporterBindings::SetVersionCrashValue(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid argument to 'setVersionCrashValue'"));
    return;
  }

  std::string value(*v8::String::Utf8Value(args[0]));
  MuonCrashReporterClient::SetVersionCrashValue(value);
}

}  // namespace brave
