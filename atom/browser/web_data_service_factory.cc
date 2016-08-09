// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/web_data_service_factory.h"

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/glue/sync_start_util.h"
#include "components/autofill/core/browser/webdata/autofill_webdata_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/webdata_services/web_data_service_wrapper.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

WebDataServiceFactory::WebDataServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "WebDataService",
          BrowserContextDependencyManager::GetInstance()) {
  // WebDataServiceFactory has no dependecies.
}

WebDataServiceFactory::~WebDataServiceFactory() {
}

// static
WebDataServiceWrapper* WebDataServiceFactory::GetForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  // If |access_type| starts being used for anything other than this
  // DCHECK, we need to start taking it as a parameter to
  // the *WebDataService::FromBrowserContext() functions (see above).
  DCHECK(access_type != ServiceAccessType::IMPLICIT_ACCESS ||
         !profile->IsOffTheRecord());
  return static_cast<WebDataServiceWrapper*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
scoped_refptr<autofill::AutofillWebDataService>
WebDataServiceFactory::GetAutofillWebDataForProfile(
    Profile* profile,
    ServiceAccessType access_type) {
  WebDataServiceWrapper* wrapper =
      WebDataServiceFactory::GetForProfile(profile, access_type);
  // |wrapper| can be null in Incognito mode.
  return wrapper ?
      wrapper->GetAutofillWebData() :
      scoped_refptr<autofill::AutofillWebDataService>(nullptr);
}

// static
WebDataServiceFactory* WebDataServiceFactory::GetInstance() {
  return base::Singleton<WebDataServiceFactory>::get();
}

content::BrowserContext* WebDataServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* WebDataServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  const base::FilePath& profile_path = context->GetPath();
  return new WebDataServiceWrapper(
      profile_path, g_browser_process->GetApplicationLocale(),
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::UI),
      BrowserThread::GetMessageLoopProxyForThread(BrowserThread::DB),
      sync_start_util::GetFlareForSyncableService(profile_path),
      NULL);
}

bool WebDataServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}
