// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <utility>

#include "brave/common/extensions/file_bindings.h"

#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/memory/ptr_util.h"
#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/sequenced_worker_pool.h"
#include "brave/common/converters/string16_converter.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "v8/include/v8.h"

using content::BrowserThread;

namespace brave {

namespace {

void PostWriteCallback(
    const base::Callback<void(bool success)>& callback,
    scoped_refptr<base::SequencedTaskRunner> reply_task_runner,
    bool write_success) {
  // We can't run |callback| on the current thread. Bounce back to
  // the |reply_task_runner| which is the correct sequenced thread.
  reply_task_runner->PostTask(FROM_HERE,
                              base::Bind(callback, write_success));
}

}  // namespace

FileBindings::FileBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {
  RouteFunction("WriteImportantFile",
      base::Bind(&FileBindings::WriteImportantFile, base::Unretained(this)));
}

FileBindings::~FileBindings() {
}

// static
v8::Local<v8::Object> FileBindings::API(
    extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_file", std::unique_ptr<extensions::NativeHandler>(
        new FileBindings(context)));

  v8::Local<v8::Object> file_api = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
        file_api, "writeImportant", "muon_file", "WriteImportantFile");

  return file_api;
}

void FileBindings::WriteImportantFile(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = args.GetIsolate();

  if (args.Length() < 2) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate,
        "Wrong number of arguments: expected 2 and received " + args.Length()));
    return;
  }

  base::FilePath::StringType path_name;
  if (!args[0]->IsString() ||
      !gin::Converter<base::FilePath::StringType>::FromV8(
          isolate, args[0], &path_name)) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`path` must be a string"));
    return;
  }
  base::FilePath path(path_name);
  if (!path.IsAbsolute()) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`path` must be absolute"));
  }

  if (!args[1]->IsString()) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`data` must be a string"));
    return;
  }
  std::string data = *v8::String::Utf8Value(args[1]);

  std::unique_ptr<v8::Global<v8::Function>> callback;
  if (args.Length() > 2 && args[2]->IsFunction()) {
    callback.reset(
        new v8::Global<v8::Function>(isolate, args[2].As<v8::Function>()));
  }

  auto task_runner = base::CreateSequencedTaskRunnerWithTraits(
      {base::MayBlock(), base::TaskShutdownBehavior::BLOCK_SHUTDOWN});
  base::ImportantFileWriter writer(path, task_runner);

  writer.RegisterOnNextWriteCallbacks(
      base::Closure(),
      base::Bind(
        &PostWriteCallback,
        base::Bind(&FileBindings::RunCallback, AsWeakPtr(),
            base::Passed(&callback)),
        base::SequencedTaskRunnerHandle::Get()));

  writer.WriteNow(base::MakeUnique<std::string>(data));
}

void FileBindings::RunCallback(
    std::unique_ptr<v8::Global<v8::Function>> callback, bool success) {
  if (!context()->is_valid() || !callback.get() || callback->IsEmpty())
    return;

  auto isolate = context()->isolate();
  v8::HandleScope handle_scope(isolate);

  v8::Local<v8::Value> callback_args[] = {
      v8::Boolean::New(isolate, success) };
  context()->SafeCallFunction(
      v8::Local<v8::Function>::New(isolate, *callback), 1, callback_args);
}

}  // namespace brave
