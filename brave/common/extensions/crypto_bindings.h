// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_EXTENSIONS_CRYPTO_BINDINGS_H_
#define BRAVE_COMMON_EXTENSIONS_CRYPTO_BINDINGS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "extensions/renderer/module_system.h"
#include "extensions/renderer/object_backed_native_handler.h"
#include "v8/include/v8.h"

namespace brave {

class CryptoBindings : public extensions::ObjectBackedNativeHandler {
 public:
  explicit CryptoBindings(extensions::ScriptContext* context);
  ~CryptoBindings() override;

  // ObjectBackedNativeHandler:
  void AddRoutes() override;

  static v8::Local<v8::Object> API(extensions::ScriptContext* context);
 private:
  void EncryptString(const v8::FunctionCallbackInfo<v8::Value>& args);
  void DecryptString(const v8::FunctionCallbackInfo<v8::Value>& args);

  DISALLOW_COPY_AND_ASSIGN(CryptoBindings);
};

}  // namespace brave

#endif  // BRAVE_COMMON_EXTENSIONS_CRYPTO_BINDINGS_H_
