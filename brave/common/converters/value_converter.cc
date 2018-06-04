// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "brave/common/converters/value_converter.h"

#include <memory>

#include "base/values.h"
#include "content/public/renderer/v8_value_converter.h"

using v8::Array;
using v8::External;
using v8::Function;
using v8::Isolate;
using v8::Local;
using v8::String;
using v8::Value;

namespace gin {

bool Converter<base::DictionaryValue>::FromV8(Isolate* isolate,
                                              Local<Value> val,
                                              base::DictionaryValue* out) {
  std::unique_ptr<content::V8ValueConverter>
      converter(content::V8ValueConverter::Create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(
      val, isolate->GetCurrentContext()));
  if (value && value->is_dict()) {
    out->Swap(static_cast<base::DictionaryValue*>(value.get()));
    return true;
  } else {
    return false;
  }
}

Local<Value> Converter<base::DictionaryValue>::ToV8(
    Isolate* isolate,
    const base::DictionaryValue& val) {
  std::unique_ptr<content::V8ValueConverter>
      converter(content::V8ValueConverter::Create());
  return converter->ToV8Value(&val, isolate->GetCurrentContext());
}

bool Converter<base::ListValue>::FromV8(Isolate* isolate,
                                        Local<Value> val,
                                        base::ListValue* out) {
  std::unique_ptr<content::V8ValueConverter>
      converter(content::V8ValueConverter::Create());
  std::unique_ptr<base::Value> value(converter->FromV8Value(
      val, isolate->GetCurrentContext()));
  if (value->is_list()) {
    out->Swap(static_cast<base::ListValue*>(value.get()));
    return true;
  } else {
    return false;
  }
}

Local<Value> Converter<base::ListValue>::ToV8(
    Isolate* isolate,
    const base::ListValue& val) {
  std::unique_ptr<content::V8ValueConverter>
      converter(content::V8ValueConverter::Create());
  return converter->ToV8Value(&val, isolate->GetCurrentContext());
}

Local<Value> Converter<Local<String> >::ToV8(Isolate* isolate,
                                             Local<String> val) {
  return val;
}

bool Converter<Local<String> >::FromV8(Isolate* isolate, Local<Value> val,
                                       Local<String>* out) {
  if (!val->IsString())
    return false;
  *out = Local<String>::Cast(val);
  return true;
}

Local<Value> Converter<Local<Array> >::ToV8(Isolate* isolate,
                                            Local<Array> val) {
  return val;
}

bool Converter<Local<Array> >::FromV8(Isolate* isolate,
                                      Local<Value> val,
                                      Local<Array>* out) {
  if (!val->IsArray())
    return false;
  *out = Local<Array>::Cast(val);
  return true;
}

}  // namespace gin
