// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/common/extensions/crypto_bindings.h"

#include <memory>
#include <string>

#include "base/base64.h"
#include "components/os_crypt/os_crypt.h"
#include "extensions/renderer/script_context.h"
#include "extensions/renderer/v8_helpers.h"
#include "gin/converter.h"
#include "v8/include/v8.h"

namespace brave {

CryptoBindings::CryptoBindings(
        extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {}

CryptoBindings::~CryptoBindings() {}

void CryptoBindings::AddRoutes() {
  RouteHandlerFunction(
      "EncryptString",
      base::Bind(&CryptoBindings::EncryptString, base::Unretained(this)));
  RouteHandlerFunction(
      "DecryptString",
      base::Bind(&CryptoBindings::DecryptString, base::Unretained(this)));
}

// static
v8::Local<v8::Object> CryptoBindings::API(
    extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_crypto", std::unique_ptr<extensions::NativeHandler>(
        new CryptoBindings(context)));

  v8::Local<v8::Object> crypto = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
      crypto,
      "encryptString", "muon_crypto", "EncryptString");
  context->module_system()->SetNativeLazyField(
      crypto,
      "decryptString", "muon_crypto", "DecryptString");
  return crypto;
}

void CryptoBindings::EncryptString(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = context()->isolate();
  if (args.Length() != 1) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`plaintext` is a required field"));
    return;
  }

  std::string plaintext = *v8::String::Utf8Value(args[0]);
  std::string ciphertext;
  if (OSCrypt::EncryptString(plaintext, &ciphertext)) {
    std::string encoded_cipher;
    base::Base64Encode(ciphertext, &encoded_cipher);
    args.GetReturnValue().Set(gin::ConvertToV8(isolate, encoded_cipher));
  } else {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`EncryptString` failed"));
  }
}

void CryptoBindings::DecryptString(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = context()->isolate();
  if (args.Length() != 1) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`ciphertext` is a required field"));
    return;
  }
  std::string encoded_cipher = *v8::String::Utf8Value(args[0]);
  std::string ciphertext;
  if (!base::Base64Decode(encoded_cipher, &ciphertext)) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "ciphertext should be in base64 format"));
    return;
  }
  std::string plaintext;
  if (OSCrypt::DecryptString(ciphertext, &plaintext)) {
    args.GetReturnValue().Set(gin::ConvertToV8(isolate, plaintext));
  } else {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`DecryptString` failed"));
  }
}

}  // namespace brave
