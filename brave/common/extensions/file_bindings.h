// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_FILE_BINDINGS_H_
#define BRAVE_COMMON_EXTENSIONS_FILE_BINDINGS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
class SequencedWorkerPool;
}

namespace brave {

class FileBindings : public extensions::ObjectBackedNativeHandler,
                     public base::SupportsWeakPtr<FileBindings> {
 public:
  explicit FileBindings(extensions::ScriptContext* context);
  ~FileBindings() override;

  static v8::Local<v8::Object> API(extensions::ScriptContext* context);

 private:
  void WriteImportantFile(const v8::FunctionCallbackInfo<v8::Value>& args);
  void RunCallback(
      std::unique_ptr<v8::Global<v8::Function>> holder, bool success);

  DISALLOW_COPY_AND_ASSIGN(FileBindings);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_FILE_BINDINGS_H_
