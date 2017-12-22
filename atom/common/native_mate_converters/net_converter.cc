// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/common/native_mate_converters/net_converter.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "native_mate/dictionary.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/base/upload_element_reader.h"
#include "net/base/upload_file_element_reader.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/ssl/client_cert_identity.h"
#include "net/url_request/url_request.h"

#include "atom/common/node_includes.h"

namespace mate {

// static
v8::Local<v8::Value> Converter<const net::AuthChallengeInfo*>::ToV8(
    v8::Isolate* isolate, const net::AuthChallengeInfo* val) {
  mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
  dict.Set("isProxy", val->is_proxy);
  dict.Set("scheme", val->scheme);
  dict.Set("host", val->challenger.host());
  dict.Set("port", static_cast<uint32_t>(val->challenger.port()));
  dict.Set("realm", val->realm);
  return mate::ConvertToV8(isolate, dict);
}

// static
v8::Local<v8::Value> Converter<std::unique_ptr<net::ClientCertIdentity>>::ToV8(
    v8::Isolate* isolate, const std::unique_ptr<net::ClientCertIdentity>& val) {
  return ConvertToV8(isolate, *val->certificate());
}

// static
v8::Local<v8::Value> Converter<scoped_refptr<net::X509Certificate>>::ToV8(
    v8::Isolate* isolate, const scoped_refptr<net::X509Certificate>& val) {
  return ConvertToV8(isolate, *val);
}

// static
v8::Local<v8::Value> Converter<net::X509Certificate>::ToV8(
    v8::Isolate* isolate, const net::X509Certificate& val) {
  mate::Dictionary dict(isolate, v8::Object::New(isolate));
  std::string encoded_data;
  net::X509Certificate::GetPEMEncoded(
      val.cert_buffer(), &encoded_data);
  dict.Set("data", encoded_data);
  dict.Set("issuerName", val.issuer().GetDisplayName());
  dict.Set("subjectName", val.subject().GetDisplayName());
  if (!val.subject().organization_names.empty())
    dict.Set("organizationNames", val.subject().organization_names);
  if (!val.subject().country_name.empty())
    dict.Set("countryName", val.subject().country_name);
  dict.Set("serialNumber", base::HexEncode(val.serial_number().data(),
                                           val.serial_number().size()));
  dict.Set("validStart", val.valid_start().ToDoubleT());
  dict.Set("validExpiry", val.valid_expiry().ToDoubleT());
  dict.Set("fingerprint",
           net::HashValue(
              val.CalculateFingerprint256(val.cert_buffer())).ToString());

  return dict.GetHandle();
}

v8::Local<v8::Value> Converter<const net::HttpResponseHeaders*>::ToV8(
                                    v8::Isolate* isolate,
                                    const net::HttpResponseHeaders* headers) {
  base::DictionaryValue response_headers;
  if (headers) {
    size_t iter = 0;
    std::string key;
    std::string value;
    while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
      key = base::ToLowerASCII(key);
      if (response_headers.HasKey(key)) {
        base::ListValue* values = nullptr;
        if (response_headers.GetList(key, &values))
          values->AppendString(value);
      } else {
        std::unique_ptr<base::ListValue> values(new base::ListValue());
        values->AppendString(value);
        response_headers.Set(key, std::move(values));
      }
    }
  }
  return ConvertToV8(isolate, response_headers);
}

bool Converter<net::HttpRequestHeaders>::FromV8(v8::Isolate* isolate,
                                          v8::Handle<v8::Value> val,
                                          net::HttpRequestHeaders* out) {
  net::HttpRequestHeaders request_headers;
  *out = request_headers;
  base::DictionaryValue headers;
  if (!ConvertFromV8(isolate, val, &headers))
    return false;

  for (base::DictionaryValue::Iterator it(headers); !it.IsAtEnd();
       it.Advance()) {
    std::string value;
    if (!it.value().GetAsString(&value))
      continue;

    out->SetHeader(it.key(), value);
  }
  return true;
}

}  // namespace mate

namespace atom {

void FillRequestDetails(base::DictionaryValue* details,
                        const net::URLRequest* request) {
  details->SetString("method", request->method());
  std::string url;
  if (!request->url_chain().empty()) url = request->url().spec();
  details->SetKey("url", base::Value(url));
  details->SetString("referrer", request->referrer());
  std::unique_ptr<base::ListValue> list(new base::ListValue);
  GetUploadData(list.get(), request);
  if (!list->empty())
    details->Set("uploadData", std::move(list));
}

void GetUploadData(base::ListValue* upload_data_list,
                   const net::URLRequest* request) {
  const net::UploadDataStream* upload_data = request->get_upload();
  if (!upload_data)
    return;
  const std::vector<std::unique_ptr<net::UploadElementReader>>* readers =
      upload_data->GetElementReaders();
  if (!readers)
    return;
  for (const auto& reader : *readers) {
    std::unique_ptr<base::DictionaryValue> upload_data_dict(
        new base::DictionaryValue);
    if (reader->AsBytesReader()) {
      const net::UploadBytesElementReader* bytes_reader =
          reader->AsBytesReader();
      std::unique_ptr<base::Value> bytes(
          base::Value::CreateWithCopiedBuffer(bytes_reader->bytes(),
                                              bytes_reader->length()));
      upload_data_dict->Set("bytes", std::move(bytes));
    } else if (reader->AsFileReader()) {
      const net::UploadFileElementReader* file_reader =
          reader->AsFileReader();
      auto file_path = file_reader->path().AsUTF8Unsafe();
      upload_data_dict->SetKey("file", base::Value(file_path));
    }
    upload_data_list->Append(std::move(upload_data_dict));
  }
}

}  // namespace atom
