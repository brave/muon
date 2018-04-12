// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_PATH_BINDINGS_H_
#define BRAVE_COMMON_EXTENSIONS_PATH_BINDINGS_H_

#include <string>
#include <utility>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace brave {

class PathBindings : public extensions::ObjectBackedNativeHandler {
 public:
  PathBindings(extensions::ScriptContext* context,
      extensions::SourceMap* source_map);
  ~PathBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

 private:
  void Append(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DirName(const v8::FunctionCallbackInfo<v8::Value>& args);
  void Require(const v8::FunctionCallbackInfo<v8::Value>& args);

  const extensions::SourceMap* source_map_;

  DISALLOW_COPY_AND_ASSIGN(PathBindings);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_PATH_BINDINGS_H_
