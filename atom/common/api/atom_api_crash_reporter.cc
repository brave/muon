// Copyright 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "muon/app/muon_crash_reporter_client.h"
#include "native_mate/converter.h"
#include "native_mate/dictionary.h"

#include "atom/common/node_includes.h"

namespace {

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  mate::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("setEnabled",
                 base::Bind(MuonCrashReporterClient::SetCrashReportingEnabled));
  // TODO(bridiver) - make this available in the renderer process
  dict.SetMethod("setCrashKeyValue",
                 base::Bind(MuonCrashReporterClient::SetCrashKeyValue));
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_common_crash_reporter, Initialize)
