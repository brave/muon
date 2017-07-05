// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
#define ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_

#include <memory>
#include "base/memory/ref_counted.h"
#include "native_mate/converter.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace net {
class AuthChallengeInfo;
class ClientCertIdentity;
class HttpRequestHeaders;
class HttpResponseHeaders;
class URLRequest;
class X509Certificate;
}

namespace mate {

template<>
struct Converter<const net::HttpResponseHeaders*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::HttpResponseHeaders* headers);
};

template<>
struct Converter<net::HttpRequestHeaders> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     net::HttpRequestHeaders* out);
};

template<>
struct Converter<const net::AuthChallengeInfo*> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const net::AuthChallengeInfo* val);
};

template<>
struct Converter<std::unique_ptr<net::ClientCertIdentity>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const std::unique_ptr<net::ClientCertIdentity>& val);
};

template<>
struct Converter<scoped_refptr<net::X509Certificate>> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const scoped_refptr<net::X509Certificate>& val);
};

template<>
struct Converter<net::X509Certificate> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
      const net::X509Certificate& val);
};

}  // namespace mate

namespace atom {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request);

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request);

}  // namespace atom

#endif  // ATOM_COMMON_NATIVE_MATE_CONVERTERS_NET_CONVERTER_H_
