// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef BRAVE_COMMON_CONVERTERS_VALUE_CONVERTER_H_
#define BRAVE_COMMON_CONVERTERS_VALUE_CONVERTER_H_

#include "gin/converter.h"

namespace base {
class DictionaryValue;
class ListValue;
class Value;
}

namespace gin {

template<>
struct Converter<base::Value> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::Value* out);
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const base::Value& val);
};

template<>
struct Converter<base::DictionaryValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::DictionaryValue* out);
};

template<>
struct Converter<base::ListValue> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::ListValue* out);
};

template<>
struct Converter<v8::Local<v8::String> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    v8::Local<v8::String> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::String>* out);
};

template<>
struct Converter<v8::Local<v8::Array> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                    v8::Local<v8::Array> val);
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     v8::Local<v8::Array>* out);
};

}  // namespace gin

#endif  // BRAVE_COMMON_CONVERTERS_VALUE_CONVERTER_H_
