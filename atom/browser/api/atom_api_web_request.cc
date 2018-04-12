// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_web_request.h"

#include "atom/browser/net/atom_network_delegate.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "base/files/file_path.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/features/features.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/url_request/url_request_context.h"
#include "v8/include/v8.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/api/web_request/web_request_api_helpers.h"
#endif

using content::BrowserThread;

namespace mate {

template<>
struct Converter<URLPattern> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     URLPattern* out) {
    std::string pattern;
    if (!ConvertFromV8(isolate, val, &pattern))
      return false;
    return out->Parse(pattern) == URLPattern::PARSE_SUCCESS;
  }
};

template<>
struct Converter<net::URLFetcher::RequestType> {
  static bool FromV8(v8::Isolate* isolate, v8::Handle<v8::Value> val,
                     net::URLFetcher::RequestType* out) {
    std::string type = base::ToLowerASCII(V8ToString(val));
    if (type == "get")
      *out = net::URLFetcher::RequestType::GET;
    else if (type == "post")
      *out = net::URLFetcher::RequestType::POST;
    else if (type == "head")
      *out = net::URLFetcher::RequestType::HEAD;
    else if (type == "delete_request")
      *out = net::URLFetcher::RequestType::DELETE_REQUEST;
    else if (type == "put")
      *out = net::URLFetcher::RequestType::PUT;
    else if (type == "patch")
      *out = net::URLFetcher::RequestType::PATCH;
    return true;
  }
};

}  // namespace mate

namespace atom {

namespace api {

WebRequest::WebRequest(v8::Isolate* isolate,
                       Profile* profile)
    : profile_(profile) {
  Init(isolate);
}

WebRequest::~WebRequest() {
  fetchers_.clear();
}

void WebRequest::OnURLFetchComplete(
    const net::URLFetcher* source) {
  FetchCallback callback = fetchers_[source];
  fetchers_.erase(source);

  mate::Dictionary response = mate::Dictionary::CreateEmpty(isolate());
  int response_code = source->GetResponseCode();
  response.Set("statusCode", response_code);

  std::string body;
  v8::Local<v8::Value> err = v8::Null(isolate());
  if (response_code == net::URLFetcher::ResponseCode::RESPONSE_CODE_INVALID ||
      !source->GetStatus().is_success()) {
    base::DictionaryValue dict;
    dict.SetInteger("errorCode", source->GetStatus().error());
    err = mate::ConvertToV8(isolate(), dict);
  } else {
    const net::HttpResponseHeaders* headers = source->GetResponseHeaders();
    response.Set("headers", headers);
    source->GetResponseAsString(&body);
  }

  // error, response, body
  const uint8_t* data = reinterpret_cast<const uint8_t*>(body.c_str());
  callback.Run(err, response, v8::String::NewFromOneByte(isolate(),
      data, v8::NewStringType::kNormal, body.length()).ToLocalChecked());
}

void WebRequest::Fetch(mate::Arguments* args) {
  GURL url;
  if (!args->GetNext(&url) || !url.is_valid()) {
    args->ThrowError("invalid url parameter");
    return;
  }

  net::URLFetcher::RequestType request_type = net::URLFetcher::RequestType::GET;
  net::HttpRequestHeaders headers;
  base::FilePath path;
  std::string payload;
  std::string payload_content_type;
  mate::Dictionary dict;
  if (args->GetNext(&dict)) {
    dict.Get("method", &request_type);
    dict.Get("headers", &headers);
    dict.Get("path", &path);
    if (dict.Get("payload", &payload)) {
      if (!dict.Get("payload_content_type", &payload_content_type)) {
        args->ThrowError("payload_content_type is required for payload");
      }
    }
  }

  FetchCallback callback;
  if (!args->GetNext(&callback)) {
    args->ThrowError("invalid callback parameter");
    return;
  }

  net::URLFetcher* fetcher = net::URLFetcher::Create(url, request_type, this)
      .release();
  fetcher->SetRequestContext(profile_->GetRequestContext());
  if (!payload.empty())
    fetcher->SetUploadData(payload_content_type, payload);
  if (!headers.IsEmpty())
    fetcher->SetExtraRequestHeaders(headers.ToString());
  if (!path.empty())
    fetcher->SaveResponseToFileAtPath(
        path, base::CreateSequencedTaskRunnerWithTraits(
                  {base::MayBlock(), base::TaskPriority::BACKGROUND}));
  fetcher->Start();
  fetchers_[fetcher] = FetchCallback(callback);
}

template<AtomNetworkDelegate::SimpleEvent type>
void WebRequest::SetSimpleListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::SimpleListener>(
      &AtomNetworkDelegate::SetSimpleListenerInIO, type, args);
}

template<AtomNetworkDelegate::ResponseEvent type>
void WebRequest::SetResponseListener(mate::Arguments* args) {
  SetListener<AtomNetworkDelegate::ResponseListener>(
      &AtomNetworkDelegate::SetResponseListenerInIO, type, args);
}

template<typename Listener, typename Method, typename Event>
void WebRequest::SetListenerOnIOThread(
    const scoped_refptr<net::URLRequestContextGetter>& getter,
    Method method, Event type, URLPatterns patterns, Listener listener) {
  auto delegate = static_cast<AtomNetworkDelegate*>(
      getter->GetURLRequestContext()->network_delegate());
  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
                            base::Bind(method, base::Unretained(delegate),
                            type, patterns, listener));
}

template<typename Listener, typename Method, typename Event>
void WebRequest::SetListener(Method method, Event type, mate::Arguments* args) {
  // { urls }.
  URLPatterns patterns;
  mate::Dictionary dict;
  args->GetNext(&dict) && dict.Get("urls", &patterns);

  // Function or null.
  v8::Local<v8::Value> value;
  Listener listener;
  if (!args->GetNext(&listener) &&
      !(args->GetNext(&value) && value->IsNull())) {
    args->ThrowError("Must pass null or a Function");
    return;
  }

  BrowserThread::PostTask(BrowserThread::IO, FROM_HERE,
      base::Bind(&WebRequest::SetListenerOnIOThread<Listener, Method, Event>,
        base::Unretained(this),
        scoped_refptr<net::URLRequestContextGetter>(
          profile_->GetRequestContext()),
          method, type, patterns, listener));
}

void WebRequest::HandleBehaviorChanged() {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  extension_web_request_api_helpers::ClearCacheOnNavigation();
#endif
}

// static
mate::Handle<WebRequest> WebRequest::Create(
    v8::Isolate* isolate,
    content::BrowserContext* browser_context) {
  DCHECK(browser_context);
  return mate::CreateHandle(isolate,
        new WebRequest(isolate, Profile::FromBrowserContext(browser_context)));
}

// static
void WebRequest::BuildPrototype(v8::Isolate* isolate,
                                v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "WebRequest"));
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
      .SetMethod("onBeforeRequest",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnBeforeRequest>)
      .SetMethod("onBeforeSendHeaders",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnBeforeSendHeaders>)
      .SetMethod("onHeadersReceived",
                 &WebRequest::SetResponseListener<
                    AtomNetworkDelegate::kOnHeadersReceived>)
      .SetMethod("onSendHeaders",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnSendHeaders>)
      .SetMethod("onBeforeRedirect",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnBeforeRedirect>)
      .SetMethod("onResponseStarted",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnResponseStarted>)
      .SetMethod("onCompleted",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnCompleted>)
      .SetMethod("onErrorOccurred",
                 &WebRequest::SetSimpleListener<
                    AtomNetworkDelegate::kOnErrorOccurred>)
      .SetMethod("handleBehaviorChanged",
                 &WebRequest::HandleBehaviorChanged)
      .SetMethod("fetch",
                 &WebRequest::Fetch);
}

}  // namespace api

}  // namespace atom
