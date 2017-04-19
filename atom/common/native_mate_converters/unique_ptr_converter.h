// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_UNIQUE_PTR_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_UNIQUE_PTR_CONVERTER_H_

#include <memory>
#include <vector>

#include "native_mate/converter.h"

namespace mate {

template<typename T>
struct Converter<std::vector<std::unique_ptr<T>> > {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const std::vector<std::unique_ptr<T>>& val) {
    v8::Local<v8::Array> result(
      MATE_ARRAY_NEW(isolate, static_cast<int>(val.size())));
    for (size_t i = 0; i < val.size(); ++i) {
      result->Set(static_cast<int>(i),
                  Converter<T>::ToV8(isolate, *(val[i].get())));
    }
    return result;
  }
};

}  // namespace mate

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_UNIQUE_PTR_CONVERTER_H_
