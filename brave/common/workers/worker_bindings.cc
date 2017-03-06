// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "brave/common/workers/worker_bindings.h"

#include "atom/browser/api/atom_api_app.h"
#include "brave/common/workers/v8_worker_thread.h"
#include "content/child/worker_thread_registry.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/renderer/script_context.h"
#include "v8/include/v8.h"

using content::BrowserThread;

namespace brave {

namespace {

void OnMessageInternal(const std::pair<uint8_t*, size_t>& buf) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::ValueDeserializer deserializer(
      isolate, buf.first, buf.second);
  deserializer.SetSupportsLegacyWireFormat(true);
  if (deserializer.ReadHeader(context).FromMaybe(false)) {
    v8::Local<v8::Value> message =
        deserializer.ReadValue(context).ToLocalChecked();
    v8::Local<v8::Object> global = context->Global();
    v8::Local<v8::Value> onmessage =
        global->Get(context, v8::String::NewFromUtf8(isolate, "onmessage",
                                                v8::NewStringType::kNormal)
                                 .ToLocalChecked()).ToLocalChecked();
    if (onmessage->IsFunction()) {
      v8::Local<v8::Function> onmessage_fun =
          v8::Local<v8::Function>::Cast(onmessage);

      v8::Local<v8::Value> argv[] = {message};
      (void)onmessage_fun->Call(context, global, 1, argv);
    }
  }
  free(buf.first);
}

}  // namespace

WorkerBindings::WorkerBindings(extensions::ScriptContext* context,
                                V8WorkerThread* worker)
    : extensions::ObjectBackedNativeHandler(context),
      worker_(worker) {
  RouteFunction("postMessage",
              base::Bind(&WorkerBindings::PostMessage, base::Unretained(this)));
}

WorkerBindings::~WorkerBindings() {
}

void WorkerBindings::Emit(const std::pair<uint8_t*, size_t>& buf) {
  v8::ValueDeserializer deserializer(
      worker_->app()->isolate(), buf.first, buf.second);
  deserializer.SetSupportsLegacyWireFormat(true);
  if (deserializer.ReadHeader(
      worker_->app()->isolate()->GetCurrentContext()).FromMaybe(false)) {
    v8::Local<v8::Value> val = deserializer.ReadValue(
        worker_->app()->isolate()->GetCurrentContext()).ToLocalChecked();
    worker_->app()->Emit("worker-post-message", worker_->GetThreadId(), val);
  } else {
    worker_->app()->Emit("worker-post-message-failure", worker_->GetThreadId());
  }
  free(buf.first);
}

void WorkerBindings::PostMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "'message' is a required field"));
    return;
  }

  if (args.Length() > 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "'transferList' is not supported yet"));
    return;
  }

  v8::ValueSerializer serializer(context()->isolate());
  serializer.WriteHeader();
  if (serializer.WriteValue(
      context()->v8_context(), args[0]).FromMaybe(false)) {
    std::pair<uint8_t*, size_t> buffer = serializer.Release();

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&WorkerBindings::Emit,
                  base::Unretained(this),
                  std::move(buffer)));
  }
}

// static
void WorkerBindings::OnMessage(v8::Isolate* isolate,
                                base::PlatformThreadId thread_id,
                                v8::Local<v8::Value> message) {
  v8::ValueSerializer serializer(isolate);
  serializer.WriteHeader();
  if (serializer.WriteValue(
      isolate->GetCurrentContext(), message).FromMaybe(false)) {
    std::pair<uint8_t*, size_t> buffer = serializer.Release();

    base::TaskRunner* task_runner =
        content::WorkerThreadRegistry::Instance()->GetTaskRunnerFor(thread_id);
    task_runner->PostTask(FROM_HERE,
        base::Bind(&OnMessageInternal,
        std::move(buffer)));
    return true;
  }
  return false;
}

}  // namespace brave
