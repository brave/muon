// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_SHARED_MEMORY_BINDINGS_H_
#define BRAVE_COMMON_EXTENSIONS_SHARED_MEMORY_BINDINGS_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/shared_memory_handle.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "native_mate/handle.h"
#include "native_mate/wrappable.h"
#include "v8/include/v8.h"

namespace base {
class SharedMemory;
}

namespace mate {

// static
template<>
struct Converter<base::SharedMemory*> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate, base::SharedMemory* val);

  static bool FromV8(v8::Isolate* isolate,
      v8::Local<v8::Value> val,
      base::SharedMemory** out);
};

}  // namespace mate

namespace brave {

class SharedMemoryWrapper : public mate::Wrappable<SharedMemoryWrapper> {
 public:
  static mate::Handle<SharedMemoryWrapper> CreateFrom(
    v8::Isolate* isolate, const base::SharedMemoryHandle& shared_memory_handle);
  static mate::Handle<SharedMemoryWrapper> CreateFrom(
    v8::Isolate* isolate, v8::Local<v8::Value> val);

  static void BuildPrototype(v8::Isolate* isolate,
                      v8::Local<v8::FunctionTemplate> prototype);

  void Close();
  base::SharedMemory* shared_memory() const { return shared_memory_.get(); }
 private:
  SharedMemoryWrapper(v8::Isolate* isolate,
      const base::SharedMemoryHandle& shared_memory_handle);
  ~SharedMemoryWrapper() override;

  std::unique_ptr<base::SharedMemory> shared_memory_;
  v8::Isolate* isolate_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryWrapper);
};

class SharedMemoryBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit SharedMemoryBindings(extensions::ScriptContext* context);
  ~SharedMemoryBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  static v8::Local<v8::Object> API(extensions::ScriptContext* context);

 private:
  void Create(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryBindings);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_SHARED_MEMORY_BINDINGS_H_
