// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "brave/common/extensions/path_bindings.h"

#include "base/files/file_path.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/source_map.h"
#include "extensions/renderer/v8_helpers.h"
#include "v8/include/v8.h"

namespace brave {

PathBindings::PathBindings(
        extensions::ScriptContext* context,
        extensions::SourceMap* source_map)
    : extensions::ObjectBackedNativeHandler(context),
      source_map_(source_map) {}

PathBindings::~PathBindings() {}

void PathBindings::AddRoutes() {
  RouteHandlerFunction("append",
                base::Bind(&PathBindings::Append, base::Unretained(this)));
  RouteHandlerFunction("dirname",
                base::Bind(&PathBindings::DirName, base::Unretained(this)));
  RouteHandlerFunction("require",
                base::Bind(&PathBindings::Require, base::Unretained(this)));
  // TODO(bridiver) - implement require.paths
}

void PathBindings::Append(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 2 || !args[0]->IsString() || !args[1]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid arguments to 'append'"));
    return;
  }

  std::string base_path_str(*v8::String::Utf8Value(args[0]));
  base::FilePath base_path(base::FilePath::FromUTF8Unsafe(base_path_str));

  std::string append_path_str(*v8::String::Utf8Value(args[1]));
  base::FilePath append_path(base::FilePath::FromUTF8Unsafe(append_path_str));

  args.GetReturnValue().Set(v8::String::NewFromUtf8(
      GetIsolate(), base_path.Append(append_path).AsUTF8Unsafe().c_str()));
}

void PathBindings::DirName(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid arguments to 'dirname'"));
    return;
  }

  std::string path_str(*v8::String::Utf8Value(args[0]));
  base::FilePath path(base::FilePath::FromUTF8Unsafe(path_str));

  args.GetReturnValue().Set(v8::String::NewFromUtf8(
      GetIsolate(), path.DirName().AsUTF8Unsafe().c_str()));
}

void PathBindings::Require(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1 || !args[0]->IsString()) {
    GetIsolate()->ThrowException(v8::String::NewFromUtf8(
        GetIsolate(), "Invalid arguments to 'require'"));
    return;
  }

  args.GetReturnValue().Set(
    source_map_->Contains(*v8::String::Utf8Value(args[0])));
}

}  // namespace brave
