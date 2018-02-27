// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "brave/common/extensions/shared_memory_bindings.h"

#include "base/memory/shared_memory.h"
#include "base/pickle.h"
#include "content/child/child_thread_impl.h"
#include "content/public/child/child_thread.h"
#include "extensions/renderer/script_context.h"
#include "native_mate/arguments.h"
#include "native_mate/converter.h"
#include "native_mate/object_template_builder.h"
#include "native_mate/wrappable.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

#include "content/public/renderer/render_thread.h"

using content::ChildThread;
using content::ChildThreadImpl;

namespace mate {

v8::Local<v8::Value> Converter<base::SharedMemory*>::ToV8(
    v8::Isolate* isolate, base::SharedMemory* val) {
  if (!val) {
    return v8::Null(isolate);
  }

  if (!val->Map(sizeof(base::Pickle::Header)))
    return v8::Null(isolate);

  // Get the payload size
  base::Pickle::Header* pickle_header =
      reinterpret_cast<base::Pickle::Header*>(val->memory());

  // Now map in the rest of the block.
  int pickle_size =
      sizeof(base::Pickle::Header) + pickle_header->payload_size;
  val->Unmap();
  if (!val->Map(pickle_size))
    return v8::Null(isolate);

  base::Pickle pickle(reinterpret_cast<char*>(val->memory()),
                      pickle_size);

  base::PickleIterator iter(pickle);
  int length = 0;
  if (!iter.ReadInt(&length))
    return v8::Null(isolate);

  const char* body = NULL;
  if (!iter.ReadBytes(&body, length))
    return v8::Null(isolate);

  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::ValueDeserializer deserializer(
      isolate, reinterpret_cast<const uint8_t*>(body), length);
  deserializer.SetSupportsLegacyWireFormat(true);
  if (deserializer.ReadHeader(context).FromMaybe(false)) {
    v8::Local<v8::Value> data =
        deserializer.ReadValue(context).ToLocalChecked();
    return data;
  }

  return v8::Null(isolate);
}

bool Converter<base::SharedMemory*>::FromV8(v8::Isolate* isolate,
    v8::Local<v8::Value> val,
    base::SharedMemory** out) {
  brave::SharedMemoryWrapper* wrapper = nullptr;
  if (!ConvertFromV8(isolate, val, &wrapper) || !wrapper)
    return false;

  *out = wrapper->shared_memory();
  return true;
}

}  // namespace mate

namespace brave {

// static
mate::Handle<SharedMemoryWrapper> SharedMemoryWrapper::CreateFrom(
    v8::Isolate* isolate,
    const base::SharedMemoryHandle& shared_memory_handle) {
  return mate::CreateHandle(
      isolate, new SharedMemoryWrapper(isolate, shared_memory_handle));
}

// static
mate::Handle<SharedMemoryWrapper> SharedMemoryWrapper::CreateFrom(
    v8::Isolate* isolate, v8::Local<v8::Value> val) {
  v8::Local<v8::Context> context = isolate->GetCurrentContext();
  v8::ValueSerializer serializer(isolate);
  serializer.WriteHeader();
  if (!serializer.WriteValue(
      context, val).FromMaybe(false)) {
    // error will be thrown by serializer
    return mate::Handle<SharedMemoryWrapper>();
  }

  std::pair<uint8_t*, size_t> buf = serializer.Release();

  base::Pickle pickle;
  pickle.WriteInt(buf.second);
  pickle.WriteBytes(buf.first, buf.second);

  // Create the shared memory object.
  std::unique_ptr<base::SharedMemory> shared_memory;
  if (ChildThread::Get()) {
    shared_memory =
        content::RenderThread::Get()->HostAllocateSharedMemoryBuffer(
                                                                 pickle.size());
  } else {
    shared_memory.reset(new base::SharedMemory);

    base::SharedMemoryCreateOptions options;
    options.size = pickle.size();
    options.share_read_only = true;
    if (!shared_memory->Create(options)) {
      free(buf.first);
      return mate::Handle<SharedMemoryWrapper>();
    }
  }

  if (!shared_memory.get() || !shared_memory->Map(pickle.size())) {
    free(buf.first);
    return mate::Handle<SharedMemoryWrapper>();
  }

  // Copy the pickle to shared memory.
  memcpy(shared_memory->memory(), pickle.data(), pickle.size());
  free(buf.first);

  base::SharedMemoryHandle handle = shared_memory->TakeHandle();

  if (!handle.IsValid()) {
    return mate::Handle<SharedMemoryWrapper>();
  }

  return CreateFrom(isolate, handle);
}

SharedMemoryWrapper::SharedMemoryWrapper(v8::Isolate* isolate,
    const base::SharedMemoryHandle& shared_memory_handle)
        : shared_memory_(new base::SharedMemory(shared_memory_handle, true)),
          isolate_(isolate) {
  Init(isolate);
}

void SharedMemoryWrapper::Close() {
  shared_memory_.reset();
}

SharedMemoryWrapper::~SharedMemoryWrapper() {}

void SharedMemoryWrapper::BuildPrototype(v8::Isolate* isolate,
                                 v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "SharedMemoryWrapper"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("close", &SharedMemoryWrapper::Close)
      .SetMethod("memory", &SharedMemoryWrapper::shared_memory);
}

SharedMemoryBindings::SharedMemoryBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {}

SharedMemoryBindings::~SharedMemoryBindings() {}

void SharedMemoryBindings::AddRoutes() {
  RouteHandlerFunction("Create", base::Bind(&SharedMemoryBindings::Create,
                                            base::Unretained(this)));
}

// static
v8::Local<v8::Object> SharedMemoryBindings::API(
    extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_shared_memory", std::unique_ptr<extensions::NativeHandler>(
        new SharedMemoryBindings(context)));

  v8::Local<v8::Object> shared_memory_api = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
        shared_memory_api, "create", "muon_shared_memory", "Create");
  return shared_memory_api;
}

void SharedMemoryBindings::Create(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() < 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "`data` is a required field"));
    return;
  }

  args.GetReturnValue().Set(
      SharedMemoryWrapper::CreateFrom(context()->isolate(), args[0]).ToV8());
}

}  // namespace brave
