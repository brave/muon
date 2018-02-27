// Copyright (c) 2017 The Brave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>

#include "brave/common/extensions/url_bindings.h"

#include "brave/common/converters/gurl_converter.h"
#include "brave/common/converters/string16_converter.h"
#include "components/url_formatter/url_formatter.h"
#include "extensions/renderer/script_context.h"
#include "gin/arguments.h"
#include "gin/converter.h"
#include "gin/dictionary.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "url/gurl.h"
#include "extensions/renderer/v8_helpers.h"
#include "v8/include/v8.h"
#include "net/base/escape.h"

using extensions::v8_helpers::SetProperty;
using extensions::v8_helpers::IsTrue;

namespace brave {

namespace {

bool SetReadOnlyProperty(v8::Local<v8::Context> context,
                        v8::Local<v8::Object> object,
                        v8::Local<v8::String> key,
                        v8::Local<v8::Value> value) {
  return IsTrue(object->DefineOwnProperty(context, key, value,
      static_cast<v8::PropertyAttribute>(v8::ReadOnly)));
}

class WrappableGURL : public GURL,
                      public gin::Wrappable<WrappableGURL> {
 public:
  explicit WrappableGURL(const GURL& other) : GURL(other) {}
  explicit WrappableGURL(base::StringPiece url_string) : GURL(url_string) {}
  explicit WrappableGURL(base::StringPiece16 url_string) : GURL(url_string) {}

  bool is_valid() const {
    return GURL::is_valid();
  }

  bool is_empty() const {
    return GURL::is_empty();
  }

  const std::string& spec() const {
    return GURL::spec();
  }

  const std::string& possibly_invalid_spec() const {
    return GURL::possibly_invalid_spec();
  }

  WrappableGURL* GetWithEmptyPath() const {
    return new WrappableGURL(GURL::GetWithEmptyPath());
  }

  WrappableGURL* GetOrigin() const {
    return new WrappableGURL(GURL::GetOrigin());
  }

  WrappableGURL* GetAsReferrer() const {
    return new WrappableGURL(GURL::GetAsReferrer());
  }

  bool IsStandard() const {
    return GURL::IsStandard();
  }

  bool SchemeIsHTTPOrHTTPS() const {
    return GURL::SchemeIsHTTPOrHTTPS();
  }

  bool SchemeIsValidForReferrer() const {
    return GURL::SchemeIsValidForReferrer();
  }

  bool SchemeIsWSOrWSS() const {
    return GURL::SchemeIsWSOrWSS();
  }

  bool SchemeIsFile() const {
    return GURL::SchemeIsFile();
  }

  bool SchemeIsFileSystem() const {
    return GURL::SchemeIsFileSystem();
  }

  bool SchemeIsCryptographic() const {
    return GURL::SchemeIsCryptographic();
  }

  bool SchemeIsBlob() const {
    return GURL::SchemeIsBlob();
  }

  bool SchemeIsSuborigin() const {
    return GURL::SchemeIsSuborigin();
  }

  std::string GetContent() const {
    return GURL::GetContent();
  }

  bool HostIsIPAddress() const {
    return GURL::HostIsIPAddress();
  }

  bool has_scheme() const {
    return GURL::has_scheme();
  }

  std::string scheme() const {
    return GURL::scheme();
  }

  bool has_host() const {
    return GURL::has_host();
  }

  std::string host() const {
    return GURL::host();
  }

  bool has_username() const {
    return GURL::has_username();
  }

  std::string username() const {
    return GURL::username();
  }

  bool has_password() const {
    return GURL::has_password();
  }

  std::string password() const {
    return GURL::password();
  }

  bool has_port() const {
    return GURL::has_port();
  }

  std::string port() const {
    return GURL::port();
  }

  bool has_path() const {
    return GURL::has_path();
  }

  std::string path() const {
    return GURL::path();
  }

  bool has_query() const {
    return GURL::has_query();
  }

  std::string query() const {
    return GURL::query();
  }

  bool has_ref() const {
    return GURL::has_ref();
  }

  std::string ref() const {
    return GURL::ref();
  }

  int EffectiveIntPort() const {
    return GURL::EffectiveIntPort();
  }

  int IntPort() const {
    return GURL::IntPort();
  }

  std::string ExtractFileName() const {
    return GURL::ExtractFileName();
  }

  std::string PathForRequest() const {
    return GURL::PathForRequest();
  }

  std::string HostNoBrackets() const {
    return GURL::HostNoBrackets();
  }

  WrappableGURL* inner_url() const {
    return new WrappableGURL(*GURL::inner_url());
  }

  bool DomainIs(std::string lower_ascii_domain) const {
    return GURL::DomainIs(lower_ascii_domain);
  }

  WrappableGURL* Resolve(const std::string& relative) const {
    return new WrappableGURL(GURL::Resolve(relative));
  }

  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override {
    return gin::Wrappable<WrappableGURL>::GetObjectTemplateBuilder(isolate)
        .SetMethod("isValid", &WrappableGURL::is_valid)
        .SetMethod("isEmpty", &WrappableGURL::is_empty)
        .SetMethod("spec", &WrappableGURL::spec)
        .SetMethod("possiblyInvalidSpec", &WrappableGURL::possibly_invalid_spec)
        .SetMethod("resolve", &WrappableGURL::Resolve)
        .SetMethod("getWithEmptyPath", &WrappableGURL::GetWithEmptyPath)
        .SetMethod("getOrigin", &WrappableGURL::GetOrigin)
        .SetMethod("getAsReferrer", &WrappableGURL::GetAsReferrer)
        .SetMethod("isStandard", &WrappableGURL::IsStandard)
        .SetMethod("schemeIsHTTPOrHTTPS", &WrappableGURL::SchemeIsHTTPOrHTTPS)
        .SetMethod("schemeIsValidForReferrer",
            &WrappableGURL::SchemeIsValidForReferrer)
        .SetMethod("schemeIsWSOrWSS", &WrappableGURL::SchemeIsWSOrWSS)
        .SetMethod("schemeIsFile", &WrappableGURL::SchemeIsFile)
        .SetMethod("schemeIsFileSystem", &WrappableGURL::SchemeIsFileSystem)
        .SetMethod("schemeIsCryptographic",
            &WrappableGURL::SchemeIsCryptographic)
        .SetMethod("schemeIsBlob", &WrappableGURL::SchemeIsBlob)
        .SetMethod("schemeIsSuborigin", &WrappableGURL::SchemeIsSuborigin)
        .SetMethod("getContent", &WrappableGURL::GetContent)
        .SetMethod("hostIsIPAddress", &WrappableGURL::HostIsIPAddress)
        .SetMethod("hasScheme", &WrappableGURL::has_scheme)
        .SetMethod("scheme", &WrappableGURL::scheme)
        .SetMethod("hasUsername", &WrappableGURL::has_username)
        .SetMethod("username", &WrappableGURL::username)
        .SetMethod("hasPassword", &WrappableGURL::has_password)
        .SetMethod("password", &WrappableGURL::password)
        .SetMethod("hasHost", &WrappableGURL::has_host)
        .SetMethod("host", &WrappableGURL::host)
        .SetMethod("hasPort", &WrappableGURL::has_port)
        .SetMethod("port", &WrappableGURL::port)
        .SetMethod("hasPath", &WrappableGURL::has_path)
        .SetMethod("path", &WrappableGURL::path)
        .SetMethod("hasQuery", &WrappableGURL::has_query)
        .SetMethod("query", &WrappableGURL::query)
        .SetMethod("hasRef", &WrappableGURL::has_ref)
        .SetMethod("ref", &WrappableGURL::ref)
        .SetMethod("effectiveIntPort", &WrappableGURL::EffectiveIntPort)
        .SetMethod("intPort", &WrappableGURL::IntPort)
        .SetMethod("extractFileName", &WrappableGURL::ExtractFileName)
        .SetMethod("pathForRequest", &WrappableGURL::PathForRequest)
        .SetMethod("hostNoBrackets", &WrappableGURL::HostNoBrackets)
        .SetMethod("domainIs", &WrappableGURL::DomainIs)
        .SetMethod("innerUrl", &WrappableGURL::inner_url);
  }

  static gin::WrapperInfo kWrapperInfo;
};

gin::WrapperInfo WrappableGURL::kWrapperInfo = { gin::kEmbedderNativeGin };

}  // namespace

URLBindings::URLBindings(extensions::ScriptContext* context)
    : extensions::ObjectBackedNativeHandler(context) {}

URLBindings::~URLBindings() {}

void URLBindings::AddRoutes() {
  RouteHandlerFunction("New",
                       base::Bind(&URLBindings::New, base::Unretained(this)));
  RouteHandlerFunction(
      "FormatForDisplay",
      base::Bind(&URLBindings::FormatForDisplay, base::Unretained(this)));
  RouteHandlerFunction("Parse",
                       base::Bind(&URLBindings::Parse, base::Unretained(this)));
}

// static
v8::Local<v8::Object> URLBindings::API(extensions::ScriptContext* context) {
  context->module_system()->RegisterNativeHandler(
    "muon_url", std::unique_ptr<extensions::NativeHandler>(
        new URLBindings(context)));

  v8::Local<v8::Object> url_api = v8::Object::New(context->isolate());
  context->module_system()->SetNativeLazyField(
        url_api, "new", "muon_url", "New");
  context->module_system()->SetNativeLazyField(
        url_api, "formatForDisplay", "muon_url", "FormatForDisplay");
  context->module_system()->SetNativeLazyField(
        url_api, "parse", "muon_url", "Parse");

  v8::Local<v8::Context> v8_context = context->v8_context();
  v8::Isolate* isolate = v8_context->GetIsolate();

  // Define unescape rule enum types (see src/net/base/escape.h)
  v8::Local<v8::Object> unescape_rules = v8::Object::New(isolate);
  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "NONE",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::NONE));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "NORMAL",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::NORMAL));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "SPACES",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::SPACES));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "PATH_SEPARATORS",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::PATH_SEPARATORS));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate,
        "URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate,
          net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "SPOOFING_AND_CONTROL_CHARS",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::SPOOFING_AND_CONTROL_CHARS));

  SetReadOnlyProperty(v8_context, unescape_rules,
      v8::String::NewFromUtf8(isolate, "REPLACE_PLUS_WITH_SPACE",
          v8::NewStringType::kNormal).ToLocalChecked(),
      gin::Converter<net::UnescapeRule::Type>::ToV8(
          isolate, net::UnescapeRule::REPLACE_PLUS_WITH_SPACE));

  // Expose Unescape rules
  SetReadOnlyProperty(v8_context, url_api,
      v8::String::NewFromUtf8(isolate, "unescapeRules",
          v8::NewStringType::kNormal).ToLocalChecked(),
      unescape_rules);

  return url_api;
}

void URLBindings::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
  if (args.Length() != 1) {
    context()->isolate()->ThrowException(v8::String::NewFromUtf8(
        context()->isolate(), "`url` is a required field"));
    return;
  }

  std::string url_string = *v8::String::Utf8Value(args[0]);
  gin::Handle<WrappableGURL> gurl = gin::CreateHandle(
      context()->isolate(), new WrappableGURL(url_string));
  args.GetReturnValue().Set(
      gurl->GetWrapper(context()->isolate()).ToLocalChecked());
}

void URLBindings::FormatForDisplay(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = context()->isolate();

  if (args.Length() < 1) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`url` is a required field"));
    return;
  }
  std::string url_string = *v8::String::Utf8Value(args[0]);

  net::UnescapeRule::Type unescape_rules = net::UnescapeRule::NONE;
  if (args.Length() > 1) {
    if (!gin::Converter<net::UnescapeRule::Type>::FromV8(
          isolate, args[1], &unescape_rules)) {
      isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`unescape_rules` has an invalid value"));
    }
  }

  base::string16 formatted_url = url_formatter::FormatUrl(GURL(url_string),
      url_formatter::kFormatUrlOmitUsernamePassword,
      unescape_rules, nullptr, nullptr, nullptr);

  args.GetReturnValue().Set(gin::ConvertToV8(isolate, formatted_url));
}

// returns a mostly node Url.parse compatible object for backwards compat
void URLBindings::Parse(
    const v8::FunctionCallbackInfo<v8::Value>& args) {
  auto isolate = context()->isolate();
  if (args.Length() != 1) {
    isolate->ThrowException(v8::String::NewFromUtf8(
        isolate, "`url` is a required field"));
    return;
  }

  std::string url_string = *v8::String::Utf8Value(args[0]);

  GURL gurl(url_string);
  gin::Dictionary dict = gin::Dictionary::CreateEmpty(isolate);
  if (gurl.is_valid()) {
    if (gurl.has_username())
      dict.Set("auth", gurl.username() + (gurl.has_password()
        ? ":" + gurl.password() : ""));
    dict.Set("hash", (gurl.has_ref() ? "#" : "") + gurl.ref());
    dict.Set("hostname", gurl.host());
    dict.Set("host", gurl.host() + (gurl.has_port() ? ":" + gurl.port() : ""));
    dict.Set("href", gurl.possibly_invalid_spec());
    dict.Set("path", gurl.PathForRequest());
    dict.Set("pathname", gurl.path());
    dict.Set("port", gurl.port());
    dict.Set("protocol", gurl.scheme() + (gurl.has_scheme() ? ":" : ""));
    dict.Set("query", gurl.query());
    dict.Set("search", (gurl.has_query() ? "?" : "") + gurl.query());
    dict.Set("origin", gurl.GetOrigin());
  }
  args.GetReturnValue().Set(gin::ConvertToV8(isolate, dict));
}

}  // namespace brave
