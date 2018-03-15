// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "brave/common/workers/worker_bindings.h"

#include "atom/browser/api/atom_api_app.h"
#include "brave/common/workers/v8_worker_thread.h"
#include "content/public/browser/browser_thread.h"
#include "content/renderer/worker_thread_registry.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "v8/include/v8.h"

using content::BrowserThread;
using extensions::v8_helpers::IsTrue;

namespace brave {

namespace {

bool SetReadOnlyProperty(v8::Local<v8::Context> context,
                        v8::Local<v8::Object> object,
                        v8::Local<v8::String> key,
                        v8::Local<v8::Value> value) {
  return IsTrue(object->DefineOwnProperty(context, key, value,
      static_cast<v8::PropertyAttribute>(v8::ReadOnly)));
}

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
      worker_(worker),
      weak_ptr_factory_(this) {}

WorkerBindings::~WorkerBindings() {}

void WorkerBindings::AddRoutes() {
  RouteHandlerFunction(
      "postMessage",
      base::Bind(&WorkerBindings::PostMessage, weak_ptr_factory_.GetWeakPtr()));
  RouteHandlerFunction("close", base::Bind(&WorkerBindings::Close,
                                           weak_ptr_factory_.GetWeakPtr()));
  RouteHandlerFunction("onerror", base::Bind(&WorkerBindings::OnError,
                                             weak_ptr_factory_.GetWeakPtr()));

  v8::Local<v8::Context> v8_context = context()->v8_context();
  v8::Isolate* isolate = v8_context->GetIsolate();

  // onmessage handler
  auto maybe_set = v8_context->Global()->CreateDataProperty(
      v8_context,
      v8::String::NewFromUtf8(isolate, "onmessage", v8::NewStringType::kNormal)
          .ToLocalChecked(),
      v8::Null(isolate));
  DCHECK(maybe_set.IsJust() && maybe_set.FromJust());

  // pathname
  v8::Local<v8::Object> location = v8::Object::New(isolate);
  SetReadOnlyProperty(v8_context, location,
      v8::String::NewFromUtf8(isolate, "pathname",
          v8::NewStringType::kNormal).ToLocalChecked(),
      v8::String::NewFromUtf8(isolate, worker_->module_name().c_str(),
          v8::NewStringType::kNormal).ToLocalChecked());

  // read-only props
  SetReadOnlyProperty(v8_context, v8_context->Global(),
      v8::String::NewFromUtf8(isolate, "self",
          v8::NewStringType::kNormal).ToLocalChecked(),
      v8_context->Global());
  SetReadOnlyProperty(v8_context, v8_context->Global(),
      v8::String::NewFromUtf8(isolate, "navigator",
          v8::NewStringType::kNormal).ToLocalChecked(),
      v8::Object::New(isolate));
  SetReadOnlyProperty(v8_context, v8_context->Global(),
      v8::String::NewFromUtf8(isolate, "location",
          v8::NewStringType::kNormal).ToLocalChecked(),
      location);
  SetReadOnlyProperty(v8_context, v8_context->Global(),
      v8::String::NewFromUtf8(isolate, "performance",
          v8::NewStringType::kNormal).ToLocalChecked(),
      v8::Object::New(isolate));

  v8::Local<v8::Object> process = v8::Object::New(isolate);
  SetReadOnlyProperty(v8_context, v8_context->Global(),
      v8::String::NewFromUtf8(isolate, "process",
          v8::NewStringType::kNormal).ToLocalChecked(),
      process);
  SetReadOnlyProperty(v8_context, process,
      v8::String::NewFromUtf8(isolate, "type",
          v8::NewStringType::kNormal).ToLocalChecked(),
      v8::String::NewFromUtf8(isolate, "worker",
          v8::NewStringType::kNormal).ToLocalChecked());

  context()->module_system()->SetNativeLazyField(
      v8_context->Global(), "postMessage", "worker", "postMessage");
  context()->module_system()->SetNativeLazyField(v8_context->Global(), "close",
                                                 "worker", "close");
  context()->module_system()->SetNativeLazyField(
      v8_context->Global(), "onerror", "worker", "onerror");
}

void WorkerBindings::OnErrorOnUIThread(const std::string& message,
                                        const std::string& stack) {
  worker_->app()->Emit(
      "worker-onerror", worker_->GetThreadId(), message, stack);
}

void WorkerBindings::OnError(
    const v8::FunctionCallbackInfo<v8::Value>& args) {

  std::string message = "Error message not available";
  std::string stack_trace = "<stack trace unavailable>";
  if (args[0]->IsObject()) {
    v8::Local<v8::Value> msg = args[0].As<v8::Object>()->Get(
        context()->v8_context(),
        v8::String::NewFromUtf8(context()->isolate(), "message",
            v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();

    if (msg->IsString())
      message = *v8::String::Utf8Value(msg);

    v8::Local<v8::Value> stack = args[0].As<v8::Object>()->Get(
        context()->v8_context(),
        v8::String::NewFromUtf8(context()->isolate(), "stack",
            v8::NewStringType::kNormal).ToLocalChecked()).ToLocalChecked();

    if (stack->IsString())
      stack_trace = *v8::String::Utf8Value(stack);
  }

  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
      base::Bind(&WorkerBindings::OnErrorOnUIThread,
                  weak_ptr_factory_.GetWeakPtr(),
                  std::move(message),
                  std::move(stack_trace)));
}

void WorkerBindings::Close(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  content::WorkerThreadRegistry::Instance()->
      GetTaskRunnerFor(worker_->GetThreadId())->PostTask(
          FROM_HERE, base::Bind(&brave::V8WorkerThread::Shutdown));
}

void WorkerBindings::PostMessageOnUIThread(
    const std::pair<uint8_t*, size_t>& buf) {
  v8::ValueDeserializer deserializer(
      worker_->app()->isolate(), buf.first, buf.second);
  deserializer.SetSupportsLegacyWireFormat(true);
  if (deserializer.ReadHeader(
      worker_->app()->isolate()->GetCurrentContext()).FromMaybe(false)) {
    v8::Local<v8::Value> val = deserializer.ReadValue(
        worker_->app()->isolate()->GetCurrentContext()).ToLocalChecked();
    worker_->app()->Emit("worker-post-message", worker_->GetThreadId(), val);
  } else {
    worker_->app()->Emit("worker-onerror", worker_->GetThreadId(),
        "`postMessage` could not deserialize message buffer");
  }
  free(buf.first);
}

void WorkerBindings::PostMessage(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "`message` is a required field"));
    return;
  }

  if (args.Length() > 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "`transferList` is not supported yet"));
    return;
  }

  v8::ValueSerializer serializer(context()->isolate());
  serializer.WriteHeader();
  if (serializer.WriteValue(
      context()->v8_context(), args[0]).FromMaybe(false)) {
    std::pair<uint8_t*, size_t> buffer = serializer.Release();

    BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
        base::Bind(&WorkerBindings::PostMessageOnUIThread,
                    weak_ptr_factory_.GetWeakPtr(),
                    std::move(buffer)));
  } else {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "`postMessage` could not serialize message"));
  }
}

// static
bool WorkerBindings::OnMessage(v8::Isolate* isolate,
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
