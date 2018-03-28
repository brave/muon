// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
#define ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "chrome/common/custom_handlers/protocol_handler.h"
#include "content/public/browser/browser_thread.h"
#include "native_mate/arguments.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_job_factory_impl.h"

namespace base {
class DictionaryValue;
}

namespace brightray {
class URLRequestContextGetter;
}

namespace content {
class BrowserContext;
}

namespace net {
class URLRequestContextGetter;
}

class Profile;

namespace atom {

namespace api {

std::vector<std::string> GetStandardSchemes();
void RegisterStandardSchemes(const std::vector<std::string>& schemes);

class Protocol : public mate::TrackableObject<Protocol> {
 public:
  using Handler =
      base::Callback<void(const base::DictionaryValue&, v8::Local<v8::Value>)>;
  using CompletionCallback = base::Callback<void(v8::Local<v8::Value>)>;
  using BooleanCallback = base::Callback<void(bool)>;

  static mate::Handle<Protocol> Create(
      v8::Isolate* isolate, content::BrowserContext* browser_context);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Protocol(v8::Isolate* isolate, Profile* profile);
  ~Protocol();

 private:
  // Possible errors.
  enum ProtocolError {
    PROTOCOL_OK,  // no error
    PROTOCOL_FAIL,  // operation failed, should never occur
    PROTOCOL_REGISTERED,
    PROTOCOL_NOT_REGISTERED,
    PROTOCOL_INTERCEPTED,
    PROTOCOL_NOT_INTERCEPTED,
  };

  // The protocol handler that will create a protocol handler for certain
  // request job.
  template<typename RequestJob>
  class CustomProtocolHandler
      : public net::URLRequestJobFactory::ProtocolHandler {
   public:
    CustomProtocolHandler(
        v8::Isolate* isolate,
        net::URLRequestContextGetter* request_context,
        const Handler& handler)
        : isolate_(isolate),
          request_context_(request_context),
          handler_(handler) {}
    ~CustomProtocolHandler() override {}

    net::URLRequestJob* MaybeCreateJob(
        net::URLRequest* request,
        net::NetworkDelegate* network_delegate) const override {
      RequestJob* request_job = new RequestJob(request, network_delegate);
      request_job->SetHandlerInfo(isolate_, request_context_.get(), handler_);
      return request_job;
    }

   private:
    v8::Isolate* isolate_;
    scoped_refptr<net::URLRequestContextGetter> request_context_;
    Protocol::Handler handler_;

    DISALLOW_COPY_AND_ASSIGN(CustomProtocolHandler);
  };

  // Register schemes that can handle service worker.
  void RegisterServiceWorkerSchemes(const std::vector<std::string>& schemes);

  // Register the protocol with certain request job.
  template<typename RequestJob>
  void RegisterProtocol(const std::string& scheme,
                        const Handler& handler,
                        mate::Arguments* args);

  template<typename RequestJob>
  static ProtocolError RegisterProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      v8::Isolate* isolate,
      const std::string& scheme,
      const Handler& handler);

  // Unregister the protocol handler that handles |scheme|.
  void UnregisterProtocol(const std::string& scheme, mate::Arguments* args);
  static ProtocolError UnregisterProtocolInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  // Whether the protocol has handler registered.
  void IsProtocolHandled(const std::string& scheme,
                         const BooleanCallback& callback);
  static bool IsProtocolHandledInIO(
      scoped_refptr<brightray::URLRequestContextGetter> request_context_getter,
      const std::string& scheme);

  const base::ListValue* GetNavigatorHandlers();
  void UnregisterNavigatorHandler(const std::string& scheme,
      const std::string& spec);
  void RegisterNavigatorHandler(const std::string& scheme,
      const std::string& spec);
  bool IsNavigatorProtocolHandled(const std::string &scheme);

  // Convert error code to JS exception and call the callback.
  void OnIOCompleted(const CompletionCallback& callback, ProtocolError error);

  // Convert error code to string.
  std::string ErrorCodeToString(ProtocolError error);

  base::WeakPtr<Protocol> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

  Profile* profile_;  // not owned
  scoped_refptr<brightray::URLRequestContextGetter> request_context_getter_;
  base::WeakPtrFactory<Protocol> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_PROTOCOL_H_
