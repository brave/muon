// Copyright 2016 Chrome authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chrome_web_ui_controller_factory.h"

#include "base/files/file_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/crashes_ui.h"
#include "chrome/common/url_constants.h"
#include "components/favicon/core/favicon_service.h"
#include "content/browser/webui/web_ui_impl.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

using content::BrowserThread;
using content::WebUI;
using content::WebUIController;

namespace {

const char kHttpNotFound[] = "HTTP/1.1 404 Not Found\n\n";

class BraveDataSource : public content::URLDataSource,
                        public net::URLFetcherDelegate {
 public:
  BraveDataSource(
    net::URLRequestContextGetter* request_context)
    : request_context_(request_context) {
  }

  // content::URLDataSource implementation.
  std::string GetSource() const override {
    return "brave";
  }

  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const GotDataCallback& callback) override {
    GURL url = GURL("file:///" + path);
    if (!url.is_valid()) {
      DLOG(WARNING) << "Invalid webui resource: brave://" << path;
      callback.Run(
          new base::RefCountedStaticMemory(kHttpNotFound, strlen(kHttpNotFound)));
      return;
    }
    net::URLFetcher* fetcher =
        net::URLFetcher::Create(url, net::URLFetcher::GET, this).release();
    pending_[fetcher] = callback;
    fetcher->SetRequestContext(request_context_.get());
    fetcher->Start();
  }

  void OnURLFetchComplete(const net::URLFetcher* source) {
    DCHECK(source);
    PendingRequestsMap::iterator it = pending_.find(source);
    DCHECK(it != pending_.end());
    std::string response;
    source->GetResponseAsString(&response);
    delete source;
    it->second.Run(base::RefCountedString::TakeString(&response));
    pending_.erase(it);
  }

  std::string GetMimeType(const std::string& path) const override {
    const GURL url("file:///" + path);

    std::string mime_type;
    base::FilePath file_path;
    if (!net::FileURLToFilePath(url, &file_path) ||
        !net::GetMimeTypeFromFile(file_path, &mime_type)) {
      DLOG(WARNING) << "Could not get mime type for: file://" << path;
    }

    return mime_type;
  }

  bool ShouldReplaceExistingSource() const override {
    return false;
  }

  bool ShouldAddContentSecurityPolicy() const override {
    return false;
  }

  bool ShouldDenyXFrameOptions() const override {
    return false;
  }

  bool ShouldServeMimeTypeAsContentTypeHeader() const override {
    return true;
  }

  bool ShouldServiceRequest(const GURL& url,
                            content::ResourceContext* resource_context,
                            int render_process_id) const override {
    return url.SchemeIs(content::kChromeUIScheme) && url.host() == "brave";
  }

 private:
  ~BraveDataSource() override {}
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  using PendingRequestsMap = std::map<const net::URLFetcher*, GotDataCallback>;
  PendingRequestsMap pending_;

  DISALLOW_COPY_AND_ASSIGN(BraveDataSource);
};

class BraveWebUIController : public WebUIController {
 public:
  BraveWebUIController(WebUI* web_ui) : WebUIController(web_ui) {
    Profile* profile = Profile::FromWebUI(web_ui);
    content::URLDataSource::Add(
        profile,
        new BraveDataSource(profile->GetRequestContext()));
  }
  bool OverrideHandleWebUIMessage(const GURL& source_url,
                                  const std::string& message,
                                  const base::ListValue& args) override {
    return false;
  }

  void RenderViewCreated(content::RenderViewHost* render_view_host) {}
};

typedef WebUIController* (*WebUIFactoryFunction)(WebUI* web_ui,
                                                 const GURL& url);

// Template for defining WebUIFactoryFunction.
template<class T>
WebUIController* NewWebUI(WebUI* web_ui, const GURL& url) {
  return new T(web_ui);
}

WebUIFactoryFunction GetWebUIFactoryFunction(WebUI* web_ui,
                                             Profile* profile,
                                             const GURL& url) {
  if (!url.SchemeIs(content::kChromeUIScheme)) {
    return NULL;
  }

  if (url.host_piece() == chrome::kChromeUICrashesHost)
    return &NewWebUI<CrashesUI>;

  if (url.host() == "brave") {
    return &NewWebUI<BraveWebUIController>;
  }

  return NULL;
}

}  // namespace

WebUI::TypeID ChromeWebUIControllerFactory::GetWebUIType(
      content::BrowserContext* browser_context, const GURL& url) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  WebUIFactoryFunction function = GetWebUIFactoryFunction(NULL, profile, url);
  return function ? reinterpret_cast<WebUI::TypeID>(function) : WebUI::kNoWebUI;
}

bool ChromeWebUIControllerFactory::UseWebUIForURL(
    content::BrowserContext* browser_context, const GURL& url) const {
  return GetWebUIType(browser_context, url) != WebUI::kNoWebUI;
}

bool ChromeWebUIControllerFactory::UseWebUIBindingsForURL(
    content::BrowserContext* browser_context, const GURL& url) const {
  return UseWebUIForURL(browser_context, url);
}

WebUIController*
ChromeWebUIControllerFactory::CreateWebUIControllerForURL(
    WebUI* web_ui,
    const GURL& url) const {
  Profile* profile = Profile::FromWebUI(web_ui);
  WebUIFactoryFunction function = GetWebUIFactoryFunction(web_ui, profile, url);
  if (!function)
    return NULL;

  return (*function)(web_ui, url);
}

// static
ChromeWebUIControllerFactory* ChromeWebUIControllerFactory::GetInstance() {
  return base::Singleton<ChromeWebUIControllerFactory>::get();
}

ChromeWebUIControllerFactory::ChromeWebUIControllerFactory() {
}

ChromeWebUIControllerFactory::~ChromeWebUIControllerFactory() {
}

void ChromeWebUIControllerFactory::GetFaviconForURL(
    Profile* profile,
    const GURL& page_url,
    const std::vector<int>& desired_sizes_in_pixel,
    const favicon_base::FaviconResultsCallback& callback) const {
  std::vector<favicon_base::FaviconRawBitmapResult>* results =
      new std::vector<favicon_base::FaviconRawBitmapResult>();

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&favicon::FaviconService::FaviconResultsCallbackRunner,
                 callback, base::Owned(results)));

}
