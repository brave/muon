// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/extensions/crash_reporter_bindings.h"

#include "base/debug/dump_without_crashing.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "v8/include/v8.h"

namespace brave {

CrashReporterBindings::CrashReporterBindings(
        extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction("SetEnabled",
      base::Bind(&CrashReporterBindings::SetEnabled,
          base::Unretained(this)));
  RouteFunction("SetCrashKeyValue",
      base::Bind(&CrashReporterBindings::SetCrashKeyValue,
          base::Unretained(this)));
  RouteFunction("DumpWithoutCrashing",
      base::Bind(&CrashReporterBindings::DumpWithoutCrashing,
          base::Unretained(this)));
}

CrashReporterBindings::~CrashReporterBindings() {
}

// static
v8::Local<v8::Object> CrashReporterBindings::API(
    extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_crash_reporter", std::unique_ptr<extensions::NativeHandler>(
        new CrashReporterBindings(context)));

  v8::Local<v8::Object> crash_reporter = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
      crash_reporter,
      "setCrashKeyValue", "muon_crash_reporter", "SetCrashKeyValue");
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

void CrashReporterBindings::SetCrashKeyValue(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid arguments to 'setCrashKeyValue'"));
    return;
  }

  std::string key(*v8::String::Utf8Value(args[0]));
  std::string value(*v8::String::Utf8Value(args[1]));
  MuonCrashReporterClient::SetCrashKeyValue(key, value);
}

}  // namespace brave
